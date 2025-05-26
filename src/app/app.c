#include "app/app.h"

#include <errno.h>  // For: errno and related macros
#include <limits.h> // For: ULONG_MAX etc.
#include <stdlib.h> // For: strtoul()
#include <string.h> // For: memset()
#include <unistd.h> // For: sleep()

#include "app/sysstat.h"
#include "sensors/sensors_config.h"
#include "utils/common.h"
#include "utils/config.h"
#include "utils/log.h"

// @TODO: Add note on sensors (Sensor_t abstraction) - static vs discovery mode

#define APP_GPIO_SET_ARG_COUNT 2   // Number of arguments in gpio set command
#define APP_GPIO_GET_ARG_COUNT 1   // Number of arguments in gpio get command
#define APP_SENSOR_GET_ARG_COUNT 2 // Number of arguments in sensor get command

// Function prototypes (declarations)
STATIC void app_remove_nl(char* buf);

/**
 * @struct App_t
 * @brief Includes running flag and handlers for the server, dispatcher, hw_interfaces, gpio, sensors.
 */
typedef struct {
    // Handlers (contexts) to instances of various classes used by the controller
    Server_t server;
    Dispatcher_t dispatcher;
    HwInterface_t i2c;
    HwInterface_t spi;
    Bme280_t sens_bme280[BME280_COUNT];
    Gpio_t gpio;
    // Internal controller state
    bool running;
} App_t;

/* Shared app context! */
static App_t app_ctx;

typedef enum { APP_MSG_TYPE_INFO = 0x00, APP_MSG_TYPE_ERROR } AppMsgType_t;

// Generic function for sending PiHub messages to the client (buf has to be a NULL terminated string no longer than APP_TEMP_MSG_BUF_SIZE!)
void app_send_to_client(const ServerClient_t* client, const char* buf, AppMsgType_t type) {
    char tmp_buf[APP_TEMP_MSG_BUF_SIZE] = "";

    // Start the message with PiHub msg type
    if(type == APP_MSG_TYPE_ERROR) {
        strncat(tmp_buf, APP_PIHUB_ERROR_MSG, APP_TEMP_MSG_BUF_SIZE - 1);
    } else {
        strncat(tmp_buf, APP_PIHUB_INFO_MSG, APP_TEMP_MSG_BUF_SIZE - strnlen(tmp_buf, APP_TEMP_MSG_BUF_SIZE) - 1);
    }

    // Concatenate the actual message and add a new line character
    strncat(tmp_buf, buf, APP_TEMP_MSG_BUF_SIZE - strnlen(tmp_buf, APP_TEMP_MSG_BUF_SIZE) - 1);
    strncat(tmp_buf, "\n", APP_TEMP_MSG_BUF_SIZE - strnlen(tmp_buf, APP_TEMP_MSG_BUF_SIZE) - 1);

    ServerError_t err_s = server_write(&app_ctx.server, *client, tmp_buf, strnlen(tmp_buf, APP_TEMP_MSG_BUF_SIZE));
    if(err_s != SERVER_ERR_OK) {
        log_error("server_write failed (ret: %d)", err_s);
    }
}

// Generic function for broadcasting PiHub messages to all clients (buf has to be a NULL terminated string no longer than APP_TEMP_MSG_BUF_SIZE!)
void app_broadcast(const char* buf, AppMsgType_t type) {
    char tmp_buf[APP_TEMP_MSG_BUF_SIZE] = "";

    // Start the message with PiHub msg type
    if(type == APP_MSG_TYPE_ERROR) {
        strncat(tmp_buf, APP_PIHUB_ERROR_MSG, APP_TEMP_MSG_BUF_SIZE - 1);
    } else {
        strncat(tmp_buf, APP_PIHUB_INFO_MSG, APP_TEMP_MSG_BUF_SIZE - strnlen(tmp_buf, APP_TEMP_MSG_BUF_SIZE) - 1);
    }

    // Concatenate the actual message and add a new line character
    strncat(tmp_buf, buf, APP_TEMP_MSG_BUF_SIZE - strnlen(tmp_buf, APP_TEMP_MSG_BUF_SIZE) - 1);
    strncat(tmp_buf, "\n", APP_TEMP_MSG_BUF_SIZE - strnlen(tmp_buf, APP_TEMP_MSG_BUF_SIZE) - 1);

    ServerError_t err_s = server_broadcast(&app_ctx.server, tmp_buf, strnlen(tmp_buf, APP_TEMP_MSG_BUF_SIZE));
    if(err_s != SERVER_ERR_OK) {
        log_error("server_write failed (ret: %d)", err_s);
    }
}

/************* Event handlers for Dispatcher *************/

void handle_gpio_set(char** argv, uint32_t argc, const void* cmd_ctx) {
    if(!cmd_ctx) {
        log_error("NULL context provided to handle_gpio_set");
        return;
    }

    // The cmd context carries details about the client that invoked the command
    ServerClient_t* client = (ServerClient_t*)cmd_ctx;

    char ip_str[IPV4_ADDRSTR_LENGTH];
    if(server_get_client_ip(*client, ip_str) == SERVER_ERR_OK) {
        log_info("'gpio set' cmd received (client IP: %.16s)", ip_str);
    } else {
        log_info("'gpio set' cmd received (client IP: failed to retrieve)");
    }

    uint8_t line, state;
    char* conversion_end_ptr;

    if(argc != APP_GPIO_SET_ARG_COUNT) {
        log_error("incorrect number of args in the 'gpio set' cmd");
        app_send_to_client(client, "incorrect number of arguments [use server help for manual]", APP_MSG_TYPE_ERROR);
        return;
    }

    // Try converting the first parameter into the line number
    errno = 0;

    unsigned long line_ul = strtoul(*argv, &conversion_end_ptr, 10);
    if(errno == EINVAL || errno == ERANGE || conversion_end_ptr == *argv) {
        log_error("failed to convert line num str into a number (errno: %s)", strerror(errno));
        app_send_to_client(client, "failed to convert line number", APP_MSG_TYPE_ERROR);
        return;
    } else if(line_ul >= UINT8_MAX) {
        log_error("line number outside the supported range (val: %lu)", line_ul);
        app_send_to_client(client, "line number outside the supported range", APP_MSG_TYPE_ERROR);
        return;
    }
    line = (uint8_t)line_ul; // line_ul is between 0 and UINT8_MAX so it's safe to cast

    // Try converting the second parameter into the state
    errno = 0;
    unsigned long state_ul = strtoul(*(argv + 1), &conversion_end_ptr, 10);
    if(errno == EINVAL || errno == ERANGE || conversion_end_ptr == *(argv + 1)) {
        log_error("failed to convert state num str into a number (errno: %s)", strerror(errno));
        app_send_to_client(client, "failed to convert state number", APP_MSG_TYPE_ERROR);
        return;
    } else if(state_ul != 0 && state_ul != 1) {
        log_error("incorrect state value (val: %lu)", state_ul);
        app_send_to_client(client, "incorrect state value (only 0 or 1 is allowed)", APP_MSG_TYPE_ERROR);
        return;
    }
    state = (uint8_t)state_ul; // state_ul is between 0 and UINT8_MAX so it's safe to cast

    char buf[APP_TEMP_MSG_BUF_SIZE] = "";
    GpioError_t err_g = gpio_set(&app_ctx.gpio, line, state);
    if(err_g != SYSSTAT_ERR_OK) {
        snprintf(buf, APP_TEMP_MSG_BUF_SIZE,
        "failed to set the GPIO output (line: %hu, state: %hu, gpio_set ret: %d)", line, state, err_g);
        log_error("gpio_set failed (line: %hu, state: %hu, ret: %d)", line, state, err_g);
        app_send_to_client(client, buf, APP_MSG_TYPE_ERROR);
    } else {
        snprintf(buf, APP_TEMP_MSG_BUF_SIZE, "GPIO line %hu set to %s", line, (state ? "HIGH" : "LOW"));
        log_info("GPIO line %hu set to %s", line, (state ? "HIGH" : "LOW"));
        app_send_to_client(client, buf, APP_MSG_TYPE_INFO);
    }
}

void handle_gpio_get(char** argv, uint32_t argc, const void* cmd_ctx) {
    if(!cmd_ctx) {
        log_error("NULL context provided to handle_gpio_get");
        return;
    }

    // The cmd context carries details about the client that invoked the command
    ServerClient_t* client = (ServerClient_t*)cmd_ctx;

    char ip_str[IPV4_ADDRSTR_LENGTH];
    if(server_get_client_ip(*client, ip_str) == SERVER_ERR_OK) {
        log_info("'gpio get' cmd received (client IP: %.16s)", ip_str);
    } else {
        log_info("'gpio get' cmd received (client IP: failed to retrieve)");
    }

    uint8_t line, state;
    char* conversion_end_ptr;

    if(argc != APP_GPIO_GET_ARG_COUNT) {
        log_error("incorrect number of arguments in the 'gpio get' cmd");
        app_send_to_client(client, "incorrect number of arguments [use server help for manual]", APP_MSG_TYPE_ERROR);
        return;
    }

    // Try converting the first parameter into the line number
    errno = 0;
    unsigned long line_ul = strtoul(*argv, &conversion_end_ptr, 10);
    if(errno == EINVAL || errno == ERANGE || conversion_end_ptr == *argv) {
        log_error("failed to convert line num str into a number (errno: %s)", strerror(errno));
        app_send_to_client(client, "failed to convert line number", APP_MSG_TYPE_ERROR);
        return;
    } else if(line_ul >= UINT8_MAX) {
        log_error("line number outside the supported range (val: %lu)", line_ul);
        app_send_to_client(client, "line number outside the supported range", APP_MSG_TYPE_ERROR);
        return;
    }
    line = (uint8_t)line_ul; // line_ul is between 0 and UINT8_MAX so it's safe to cast

    char buf[APP_TEMP_MSG_BUF_SIZE] = "";
    GpioError_t err_g = gpio_get(&app_ctx.gpio, line, &state);
    if(err_g != SYSSTAT_ERR_OK) {
        snprintf(buf, APP_TEMP_MSG_BUF_SIZE,
        "failed to get the GPIO output (line: %hu, state: %hu, gpio_get ret: %d)", line, state, err_g);
        log_error("gpio_get failed (line: %hu, state: %hu, ret: %d)", line, state, err_g);
        app_send_to_client(client, buf, APP_MSG_TYPE_ERROR);
    } else {
        snprintf(buf, APP_TEMP_MSG_BUF_SIZE, "GPIO line %hu is %s", line, (state ? "HIGH" : "LOW"));
        log_debug("GPIO line %hu is %s", line, (state ? "HIGH" : "LOW"));
        app_send_to_client(client, buf, APP_MSG_TYPE_INFO);
    }
}

void handle_sensor_list(char** argv, uint32_t argc, const void* cmd_ctx) {
    if(!cmd_ctx) {
        log_error("NULL context provided to handle_sensor_list");
        return;
    }

    // The cmd context carries details about the client that invoked the command
    ServerClient_t* client = (ServerClient_t*)cmd_ctx;

    char ip_str[IPV4_ADDRSTR_LENGTH];
    if(server_get_client_ip(*client, ip_str) == SERVER_ERR_OK) {
        log_info("'sensor list' cmd received (client IP: %.16s)", ip_str);
    } else {
        log_info("'sensor list' cmd received (client IP: failed to retrieve)");
    }

    char buf[APP_TEMP_MSG_BUF_SIZE] = "";

    if(BME280_COUNT <= 0) {
        app_send_to_client(client, "No sensors configured", APP_MSG_TYPE_ERROR);
    }

    // List all bme280 sensors defined in the sensors_config.h configuration file
    for(int i = 0; i < BME280_COUNT; ++i) {
        snprintf(buf, APP_TEMP_MSG_BUF_SIZE, "sensor id: #%d; addr: 0x%02hhX; hw if: %s", i,
        SENSORS_CONFIG_BME280[i].addr, (SENSORS_CONFIG_BME280[i].if_type == HW_INTERFACE_I2C ? "I2C" : "SPI"));
        app_send_to_client(client, buf, APP_MSG_TYPE_INFO);
    }
}

void handle_sensor_get(char** argv, uint32_t argc, const void* cmd_ctx) {
    if(!cmd_ctx) {
        log_error("NULL context provided to handle_sensor_get");
        return;
    }

    // The cmd context carries details about the client that invoked the command
    ServerClient_t* client = (ServerClient_t*)cmd_ctx;

    char ip_str[IPV4_ADDRSTR_LENGTH];
    if(server_get_client_ip(*client, ip_str) == SERVER_ERR_OK) {
        log_info("'sensor get' cmd received (client IP: %.16s)", ip_str);
    } else {
        log_info("'sensor get' cmd received (client IP: failed to retrieve)");
    }


    uint8_t id;
    char* conversion_end_ptr;

    if(argc != APP_SENSOR_GET_ARG_COUNT) {
        log_error("incorrect number of arguments in the 'sensor get' cmd");
        app_send_to_client(client, "incorrect number of arguments [use server help for manual]", APP_MSG_TYPE_ERROR);
        return;
    }

    // Try converting the first parameter into the sensor ID
    errno = 0;
    unsigned long sensor_id_ul = strtoul(*argv, &conversion_end_ptr, 10);
    if(errno == EINVAL || errno == ERANGE || conversion_end_ptr == *argv) {
        log_error("failed to convert sensor ID str into a number (errno: %s)", strerror(errno));
        app_send_to_client(client, "failed to convert the sensor ID", APP_MSG_TYPE_ERROR);
        return;
    } else if(sensor_id_ul >= UINT8_MAX || sensor_id_ul > BME280_COUNT) {
        log_error("sensor ID invalid (val: %lu)", sensor_id_ul);
        app_send_to_client(client, "invalid sensor ID", APP_MSG_TYPE_ERROR);
        return;
    }
    id = (uint8_t)sensor_id_ul; // sensor_id_ul is between 0 and UINT8_MAX so it's safe to cast

    AppMsgType_t resp_type = APP_MSG_TYPE_INFO;
    char buf[APP_TEMP_MSG_BUF_SIZE] = "";
    if(strncasecmp(*(argv + 1), APP_HUM_STRING, DISPATCHER_ARG_MAX_SIZE) == 0) {
        float hum;
        SensorError_t err_s = bme280_get_hum(&app_ctx.sens_bme280[id], &hum);
        if(err_s == SENSOR_ERR_OK) {
            log_debug("sensor #%hu returned humidity: %.2f %%", id, hum);
            snprintf(buf, APP_TEMP_MSG_BUF_SIZE, "sensor #%hu returned humidity: %.2f %%", id, hum);
        } else {
            log_error("bme280_get_hum failed (sensor id: %hu, ret: %d)", id, err_s);
            snprintf(buf, APP_TEMP_MSG_BUF_SIZE,
            "failed to read humidity from sensor #%hu (bme280_get_hum ret: %d)", id, err_s);
            resp_type = APP_MSG_TYPE_ERROR;
        }
    } else if(strncasecmp(*(argv + 1), APP_TEMP_STRING, DISPATCHER_ARG_MAX_SIZE) == 0) {
        float temp;
        SensorError_t err_s = bme280_get_temp(&app_ctx.sens_bme280[id], &temp);
        if(err_s == SENSOR_ERR_OK) {
            log_debug("sensor #%hu returned temp: %.2f *C", id, temp);
            snprintf(buf, APP_TEMP_MSG_BUF_SIZE, "sensor #%hu returned temp: %.2f *C", id, temp);
        } else {
            log_error("bme280_get_temp failed (sensor id: %hu, ret: %d)", id, err_s);
            snprintf(buf, APP_TEMP_MSG_BUF_SIZE,
            "failed to read temp from sensor #%hu (bme280_get_temp ret: %d)", id, err_s);
            resp_type = APP_MSG_TYPE_ERROR;
        }
    } else if(strncasecmp(*(argv + 1), APP_PRESS_STRING, DISPATCHER_ARG_MAX_SIZE) == 0) {
        float press;
        SensorError_t err_s = bme280_get_press(&app_ctx.sens_bme280[id], &press);
        if(err_s == SENSOR_ERR_OK) {
            snprintf(buf, APP_TEMP_MSG_BUF_SIZE, "sensor #%hu returned press: %.2f Pa", id, press);
            log_debug("sensor #%hu returned press: %.2f Pa", id, press);
        } else {
            log_error("bme280_get_press failed (sensor id: %hu, ret: %d)", id, err_s);
            snprintf(buf, APP_TEMP_MSG_BUF_SIZE,
            "failed to read press from sensor #%hu (bme280_get_press ret: %d)", id, err_s);
            resp_type = APP_MSG_TYPE_ERROR;
        }
    } else {
        log_error("unsupported measurement type ('%.20s')", *(argv + 1));
        snprintf(buf, APP_TEMP_MSG_BUF_SIZE, "unsupported measurement type");
        resp_type = APP_MSG_TYPE_ERROR;
    }
    app_send_to_client(client, buf, resp_type);
}

void handle_server_status(char** argv, uint32_t argc, const void* cmd_ctx) {
    if(!cmd_ctx) {
        log_error("NULL context provided to handle_server_status");
        return;
    }

    // The cmd context carries details about the client that invoked the command
    ServerClient_t* client = (ServerClient_t*)cmd_ctx;

    char ip_str[IPV4_ADDRSTR_LENGTH];
    if(server_get_client_ip(*client, ip_str) == SERVER_ERR_OK) {
        log_info("'server status' cmd received (client IP: %.16s)", ip_str);
    } else {
        log_info("'server status' cmd received (client IP: failed to retrieve)");
    }

    SysstatMemInfo_t mem_stats;
    SysstatNetInfo_t net_stats;
    SysstatUptimeInfo_t time_stats;
    char buf[APP_TEMP_MSG_BUF_SIZE] = "";

    SysstatError_t err_stat = sysstat_get_mem_info(&mem_stats);
    if(err_stat != SYSSTAT_ERR_OK) {
        log_error("sysstat_get_mem_info failed (ret: %d)", err_stat);
        sprintf(buf, "failed to retrieve memory stats (sysstat_get_mem_info ret: %d)", err_stat);
        app_send_to_client(client, buf, APP_MSG_TYPE_ERROR);
        return;
    }

    err_stat = sysstat_get_net_info(NET_INTERFACE_NAME, &net_stats);
    if(err_stat != SYSSTAT_ERR_OK) {
        log_error("sysstat_get_net_info failed (ret: %d)", err_stat);
        sprintf(buf, "failed to retrieve network stats (sysstat_get_net_info ret: %d)", err_stat);
        app_send_to_client(client, buf, APP_MSG_TYPE_ERROR);
        return;
    }

    err_stat = sysstat_get_uptime_info(&time_stats);
    if(err_stat != SYSSTAT_ERR_OK) {
        log_error("sysstat_get_uptime_info failed (ret: %d)", err_stat);
        sprintf(buf, "failed to retrieve uptime stats (sysstat_get_uptime_info ret: %d)", err_stat);
        app_send_to_client(client, buf, APP_MSG_TYPE_ERROR);
        return;
    }

    // Check the number of connected clients
    uint32_t clients_count = 0;
    for(ListNode_t* node = server_get_clients(&app_ctx.server); node != NULL; node = node->next) {
        clients_count++;
    }

    snprintf(buf, APP_TEMP_MSG_BUF_SIZE,
    "Mem %lu kB/%lu kB (available/total) | Net tx: %lu kB, rx: %lu kB | Uptime %u.%hu s", mem_stats.available_kB,
    mem_stats.total_kB, net_stats.tx_bytes / 1000, net_stats.rx_bytes / 1000, time_stats.up.s, time_stats.up.ms);
    app_send_to_client(client, buf, APP_MSG_TYPE_INFO);

    snprintf(buf, APP_TEMP_MSG_BUF_SIZE, "connected clients: %u", clients_count);
    app_send_to_client(client, buf, APP_MSG_TYPE_INFO);
}

void handle_server_uptime(char** argv, uint32_t argc, const void* cmd_ctx) {
    if(!cmd_ctx) {
        log_error("NULL context provided to the handle_server_uptime");
        return;
    }

    // The cmd context carries details about the client that invoked the command
    ServerClient_t* client = (ServerClient_t*)cmd_ctx;

    char ip_str[IPV4_ADDRSTR_LENGTH];
    if(server_get_client_ip(*client, ip_str) == SERVER_ERR_OK) {
        log_info("'server uptime' cmd received (client IP: %.16s)", ip_str);
    } else {
        log_info("'server uptime' cmd received (client IP: failed to retrieve)");
    }

    SysstatUptimeInfo_t time_stats;
    char buf[APP_TEMP_MSG_BUF_SIZE] = "";

    SysstatError_t err_stat = sysstat_get_uptime_info(&time_stats);
    if(err_stat != SYSSTAT_ERR_OK) {
        snprintf(buf, APP_TEMP_MSG_BUF_SIZE, "failed to retrieve uptime info (sysstat_get_uptime_info ret: %d)", err_stat);
        log_error("sysstat_get_uptime_info failed (ret: %d)", err_stat);
        app_send_to_client(client, buf, APP_MSG_TYPE_ERROR);
    } else {
        snprintf(buf, APP_TEMP_MSG_BUF_SIZE, "uptime %u.%hu s", time_stats.up.s, time_stats.up.ms);
        app_send_to_client(client, buf, APP_MSG_TYPE_INFO);
    }
}

void handle_server_net(char** argv, uint32_t argc, const void* cmd_ctx) {
    if(!cmd_ctx) {
        log_error("NULL context provided to the handle_server_net");
        return;
    }

    // The cmd context carries details about the client that invoked the command
    ServerClient_t* client = (ServerClient_t*)cmd_ctx;

    char ip_str[IPV4_ADDRSTR_LENGTH];
    if(server_get_client_ip(*client, ip_str) == SERVER_ERR_OK) {
        log_info("'server net' cmd received (client IP: %.16s)", ip_str);
    } else {
        log_info("'server net' cmd received (client IP: failed to retrieve)");
    }

    SysstatNetInfo_t net_stats;
    char buf[APP_TEMP_MSG_BUF_SIZE] = "";

    SysstatError_t err_stat = sysstat_get_net_info(NET_INTERFACE_NAME, &net_stats);
    if(err_stat != SYSSTAT_ERR_OK) {
        sprintf(buf, "failed to retrieve network stats (sysstat_get_net_info ret: %d)", err_stat);
        log_error("sysstat_get_net_info failed (ret: %d)", err_stat);
        app_send_to_client(client, buf, APP_MSG_TYPE_ERROR);
    } else {
        sprintf(buf, "net tx: %lu kB (%lu packets), rx: %lu kB (%lu packets)", net_stats.tx_bytes / 1000,
        net_stats.rx_packets, net_stats.rx_bytes / 1000, net_stats.tx_packets);
        app_send_to_client(client, buf, APP_MSG_TYPE_INFO);
    }
}

void handle_server_disconnect(char** argv, uint32_t argc, const void* cmd_ctx) {
    if(!cmd_ctx) {
        log_error("NULL context provided to the handle_server_disconnect");
        return;
    }

    // The cmd context carries details about the client that invoked the command
    ServerClient_t* client = (ServerClient_t*)cmd_ctx;

    char ip_str[IPV4_ADDRSTR_LENGTH];
    if(server_get_client_ip(*client, ip_str) == SERVER_ERR_OK) {
        log_info("'server disconnect' cmd received (client IP: %.16s)", ip_str);
    } else {
        log_info("'server disconnect' cmd received (client IP: failed to retrieve)");
    }

    app_send_to_client(client, "disconnecting from the server...", APP_MSG_TYPE_INFO);

    ServerError_t err_s = server_disconnect(&app_ctx.server, *client);
    if(err_s != SERVER_ERR_OK) {
        char buf[APP_TEMP_MSG_BUF_SIZE] = "";
        sprintf(buf, "failed to disconnect from the server (server_disconnect ret: %d)", err_s);
        log_error("server_disconnect failed (ret: %d)", err_s);
        app_send_to_client(client, buf, APP_MSG_TYPE_ERROR);
    }
}

void handle_server_help(char** argv, uint32_t argc, const void* cmd_ctx) {
    if(!cmd_ctx) {
        log_error("NULL context provided to handle_server_help");
        return;
    }

    ServerClient_t* client = (ServerClient_t*)cmd_ctx;

    char ip_str[IPV4_ADDRSTR_LENGTH];
    if(server_get_client_ip(*client, ip_str) == SERVER_ERR_OK) {
        log_info("'server help' cmd received (client IP: %.16s)", ip_str);
    } else {
        log_info("'server help' cmd received (client IP: failed to retrieve)");
    }

    const size_t line_count = sizeof(APP_HELP_MSG) / sizeof(APP_HELP_MSG[0]);

    for(size_t i = 0; i < line_count; ++i) {
        app_send_to_client(client, APP_HELP_MSG[i], APP_MSG_TYPE_INFO);
    }
}

/************* Event handlers for Server *************/

/* Welcome the user and notify other users about the new client (broadcast a message) */
void handle_client_connect(void* ctx, const ServerClient_t client) {
    Server_t* _ctx = (Server_t*)ctx;

    log_debug("handle_client_connect called");

    // Get the IP address of the new client
    char ip_str[IPV4_ADDRSTR_LENGTH];
    ServerError_t err_s = server_get_client_ip(client, ip_str);
    if(err_s != SERVER_ERR_OK) {
        log_error("server_get_client_ip failed (ret: %d)", err_s);
    }

    // Send a welcome message to the user
    app_send_to_client(&client, APP_WELCOME_MSG, APP_MSG_TYPE_INFO);

    // Notify other clients about the new user
    char msg_connect[APP_CONNECT_MSG_BUF_SIZE] = "";
    strncat(msg_connect, ip_str, APP_CONNECT_MSG_BUF_SIZE - 1);
    strncat(msg_connect, APP_CONNECT_MSG, APP_CONNECT_MSG_BUF_SIZE - 1);
    app_broadcast(msg_connect, APP_MSG_TYPE_INFO);
}

/* Read received data and pass it to the dispatcher for parsing and executing associated command */
void handle_data_received(void* ctx, const ServerClient_t client) {
    Server_t* _ctx = (Server_t*)ctx;
    char buf[APP_SERVER_RECV_DATA_BUF_SIZE];
    ssize_t len;

    log_debug("handle_data_received called");

    // Try reading out the received data
    ServerError_t err_s = server_read(ctx, client, buf, APP_SERVER_RECV_DATA_BUF_SIZE, &len);
    if(err_s != SERVER_ERR_OK) {
        log_error("failed to read the incomming data (err: %d)", err_s);
    }

    app_remove_nl(buf); // Remove new line character from the buffer (if present)

    // Try executing the command
    DispatcherError_t err_d = dispatcher_execute(&app_ctx.dispatcher, buf, &client);
    switch(err_d) {
    case DISPATCHER_ERR_OK: {
        break;
    }
    case DISPATCHER_ERR_CMD_INCOMPLETE: {
        app_send_to_client(&client, APP_CMD_INCOMPLETE_MSG, APP_MSG_TYPE_ERROR);
        break;
    }
    case DISPATCHER_ERR_BUF_TOO_LONG:   // fallthrough
    case DISPATCHER_ERR_BUF_EMPTY:      // fallthrough
    case DISPATCHER_ERR_TOKEN_TOO_LONG: // fallthrough
    case DISPATCHER_ERR_CMD_NOT_FOUND: {
        app_send_to_client(&client, APP_CMD_ERR_MSG, APP_MSG_TYPE_ERROR);
        break;
    }
    case DISPATCHER_ERR_NULL_ARG:        // fallthrough
    case DISPATCHER_ERR_PTHREAD_FAILURE: // fallthrough
    default: {
        app_send_to_client(&client, APP_GENERIC_FAILURE_MSG, APP_MSG_TYPE_ERROR);
        break;
    }
    }
}

/* Notify other users about the disconnection (broadcast a message) */
void handle_client_disconnect(void* ctx, const ServerClient_t client) {
    Server_t* _ctx = (Server_t*)ctx;

    log_debug("handle_client_disconnect called");

    ServerError_t err_s =
    server_broadcast(_ctx, APP_DISCONNECT_MSG, strnlen(APP_DISCONNECT_MSG, sizeof(APP_DISCONNECT_MSG)));
    if(err_s != SERVER_ERR_OK) {
        log_error("server_broadcast failed (ret: %d)", err_s);
    }
}

/* Reinit and restart the whole application controller */
void handle_server_failure(void* ctx, const ServerError_t err) {
    Server_t* _ctx = (Server_t*)ctx;

    log_info("handle_server_failure called with error: %d", err);
    log_info("attempting to restart the server");

    // Stop the app controller...
    AppError_t ret_a = app_stop();
    if(ret_a != APP_ERR_OK) {
        log_error("app_stop failed (ret: %d)", ret_a);
    }

    // Give the app some time margin before reinit is done
    sleep(10);

    // ...reinit...
    ret_a = app_deinit();
    if(ret_a != APP_ERR_OK) {
        log_error("app_deinit failed (ret: %d)", ret_a);
    }
    ret_a = app_init();
    if(ret_a != APP_ERR_OK) {
        log_error("app_init failed (ret: %d)", ret_a);
    }

    // ...and restart it from scratch
    ret_a = app_run();
    if(ret_a != APP_ERR_OK) {
        log_error("app_stop failed (ret: %d)", ret_a);
    }
}


// Initialize the server component
AppError_t app_init_server(void) {
    // Configure callbacks for the server events
    ServerCallbackList_t server_cb_list = { .on_client_connect = handle_client_connect,
        .on_client_disconnect = handle_client_disconnect,
        .on_data_received = handle_data_received,
        .on_server_failure = handle_server_failure };

    // Configure server parameters
    ServerConfig_t server_cfg = { .port = APP_SERVER_PORT,
        .cb_list = server_cb_list,
        .max_clients = APP_SERVER_MAX_CLIENTS,
        .max_conn_requests = APP_SERVER_MAX_CONN_REQUESTS };

    ServerError_t err_s = server_init(&app_ctx.server, server_cfg);
    if(err_s == SERVER_ERR_OK) {
        log_debug("server initialized successfully (port: %s, max clients: %d, max conn requests: %d)",
        APP_SERVER_PORT, APP_SERVER_MAX_CLIENTS, APP_SERVER_MAX_CONN_REQUESTS);
    } else {
        log_error("failed to initialize the server (err: %d)", err_s);
        return APP_ERR_SERVER_FAILURE;
    }

    return APP_ERR_OK;
}

// Initialize the command dispatcher
AppError_t app_init_dispatcher(void) {
    // Configure dispatcher parameters and supported commands
    const DispatcherConfig_t cfg = { .delim = APP_DISPATCHER_DELIM };

    const DispatcherCommandDef_t cmd_list[] = { // List of all commands to be supported
        { .target = "gpio", .action = "set", .callback_ptr = handle_gpio_set },
        { .target = "gpio", .action = "get", .callback_ptr = handle_gpio_get },
        { .target = "sensor", .action = "list", .callback_ptr = handle_sensor_list },
        { .target = "sensor", .action = "get", .callback_ptr = handle_sensor_get },
        { .target = "server", .action = "status", .callback_ptr = handle_server_status },
        { .target = "server", .action = "uptime", .callback_ptr = handle_server_uptime },
        { .target = "server", .action = "net", .callback_ptr = handle_server_net },
        { .target = "server", .action = "disconnect", .callback_ptr = handle_server_disconnect },
        { .target = "server", .action = "help", .callback_ptr = handle_server_help }
    };

    // Initialize the dispatcher
    DispatcherError_t err_d = dispatcher_init(&app_ctx.dispatcher, cfg);
    if(err_d == DISPATCHER_ERR_OK) {
        log_debug("dispatcher initialized successfully (delim: %s)", APP_DISPATCHER_DELIM);
    } else {
        log_error("failed to initialize the dispatcher (err: %d)", err_d);
        return APP_ERR_DISPATCHER_FAILURE;
    }

    // Register all commands listed in cmd_list[]
    for(int i = 0; i < (sizeof(cmd_list) / sizeof(DispatcherCommandDef_t)); i++) {
        err_d = dispatcher_register(&app_ctx.dispatcher, i, cmd_list[i]);
        if(err_d == DISPATCHER_ERR_OK) {
            log_debug("cmd %.30s|%.30s registered successfully", cmd_list[i].target, cmd_list[i].action);
        } else {
            log_error("failed to initialize the %.30s|%.30s cmd (err: %d)", cmd_list[i].target, cmd_list[i].action, err_d);
            return APP_ERR_DISPATCHER_FAILURE;
        }
    }

    return APP_ERR_OK;
}

// Zero out ctx, Init the server
AppError_t app_init() {
    // Zero out context on init
    memset(&app_ctx, 0, sizeof(App_t));

    // Initialize the server
    AppError_t err_app = app_init_server();
    if(err_app != APP_ERR_OK) {
        return err_app;
    }

    // Initialize the dispatcher
    err_app = app_init_dispatcher();
    if(err_app != APP_ERR_OK) {
        return err_app;
    }

    // Initialize the GPIO driver
    GpioError_t err_g = gpio_init(&app_ctx.gpio);
    if(err_g != GPIO_ERR_OK) {
        log_error("gpio_init failed (err: %d)", err_g);
        return APP_ERR_GPIO_FAILURE;
    }

    // Initialize the i2c
    HwInterfaceError_t err_hw = hw_interface_init(&app_ctx.i2c, HW_INTERFACE_I2C);
    if(err_hw != HW_INTERFACE_ERR_OK) {
        log_error("hw_interface_init failed (err: %d)", err_hw);
        return APP_ERR_HW_INTERFACE_FAILURE;
    }

    // Initialize all bme280 sensors defined in the sensors_config.h configuration file
    HwInterface_t hw_if;
    for(int i = 0; i < BME280_COUNT; ++i) {
        if(SENSORS_CONFIG_BME280[i].if_type == HW_INTERFACE_I2C) {
            hw_if = app_ctx.i2c;
        } else if(SENSORS_CONFIG_BME280[i].if_type == HW_INTERFACE_SPI) {
            hw_if = app_ctx.spi;
        }
        SensorError_t err_s = bme280_init(&app_ctx.sens_bme280[i], SENSORS_CONFIG_BME280[i].addr, hw_if);
        if(err_s != SENSOR_ERR_OK) {
            log_error("bme280_init failed (err: %d)", err_hw);
            return APP_ERR_SENSOR_FAILURE;
        }
    }

    return APP_ERR_OK;
}

// Start the server
AppError_t app_run(void) {
    if(app_ctx.running) {
        return APP_ERR_RUNNING;
    }

    ServerError_t err_s = server_run(&app_ctx.server);
    if(err_s == SERVER_ERR_OK) {
        log_debug("server started successfully");
    } else {
        log_error("failed to start the server (err: %d)", err_s);
        return APP_ERR_SERVER_FAILURE;
    }

    app_ctx.running = true;

    return APP_ERR_OK;
}

// Stop the server
AppError_t app_stop(void) {
    if(!app_ctx.running) {
        return APP_ERR_NOT_STARTED;
    }

    ServerError_t err_s = server_shutdown(&app_ctx.server);
    if(err_s == SERVER_ERR_OK) {
        log_debug("server stopped successfully");
    } else {
        log_error("failed to stop the server (err: %d)", err_s);
        return APP_ERR_SERVER_FAILURE;
    }

    app_ctx.running = false;

    return APP_ERR_OK;
}

// Deinit the server, dispatcher
AppError_t app_deinit(void) {
    if(app_ctx.running) {
        return APP_ERR_RUNNING;
    }

    // Deinit the server
    ServerError_t err_s = server_deinit(&app_ctx.server);
    if(err_s == SERVER_ERR_OK) {
        log_debug("server deinitialized successfully");
    } else {
        log_error("failed to deinitialize the server (err: %d)", err_s);
        return APP_ERR_SERVER_FAILURE;
    }

    // Deinit the dispatcher
    DispatcherError_t err_d = dispatcher_deinit(&app_ctx.dispatcher);
    if(err_d == DISPATCHER_ERR_OK) {
        log_debug("dispatcher deinitialized successfully");
    } else {
        log_error("failed to deinitialize the dispatcher (err: %d)", err_d);
        return APP_ERR_SERVER_FAILURE;
    }

    // Deinit the GPIO driver
    GpioError_t err_g = gpio_deinit(&app_ctx.gpio);
    if(err_g == GPIO_ERR_OK) {
        log_debug("gpio deinitialized successfully");
    } else {
        log_error("failed to deinitialize the gpio driver (err: %d)", err_g);
        return APP_ERR_GPIO_FAILURE;
    }

    // Deinit the i2c
    HwInterfaceError_t err_hw = hw_interface_deinit(&app_ctx.i2c);
    if(err_hw != HW_INTERFACE_ERR_OK) {
        log_error("hw_interface_deinit failed (err: %d)", err_hw);
        return APP_ERR_HW_INTERFACE_FAILURE;
    }

    // Deinitialize all bme280 sensors defined in the sensors_config.h configuration file
    for(int i = 0; i < BME280_COUNT; ++i) {
        SensorError_t err_s = bme280_deinit(&app_ctx.sens_bme280[i]);
        if(err_s != SENSOR_ERR_OK) {
            log_error("bme280_deinit failed (err: %d)", err_hw);
            return APP_ERR_SENSOR_FAILURE;
        }
    }

    // Zero-out context on deinit
    memset(&app_ctx, 0, sizeof(App_t));

    return APP_ERR_OK;
}

// Replace new line with terminating character
STATIC void app_remove_nl(char* buf) {
    // Scan the input buffer as long as its valid (non null) and terminating char has not been reached
    while(buf || *buf != '\0') {
        if(*buf == '\n') {
            *buf = '\0'; // Replace the first new line with terminating character ...
            return;      // ... and return
        }
        buf++;
    }
}