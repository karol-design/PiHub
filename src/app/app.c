#include "app/app.h"

#include <string.h> // for: memset
#include <unistd.h> // For: sleep()

#include "app/sysstat.h"
#include "utils/common.h"
#include "utils/log.h"


// Configuration macros
#define APP_SERVER_PORT "65002" // Port number (as a string) under which the PiHub server should be accessible
#define APP_SERVER_MAX_CLIENTS 5          // Maximum number of clients connected at the same time
#define APP_SERVER_MAX_CONN_REQUESTS 10   // Maximum number of pending connection reuqests
#define APP_SERVER_RECV_DATA_BUF_SIZE 128 // Size of the buffer for new data from the clients
#define NET_INTERFACE_NAME "eth0"         // Name of the network interface

#define APP_DISPATCHER_DELIM " " // Delimiter in commands handled by the dispatcher

#define APP_PIHUB_INFO_MSG "[PiHub] info: "
#define APP_PIHUB_ERROR_MSG "[PiHub] error: "
#define APP_PIHUB_PROMPT_CHAR "> "

#define APP_TEMP_MSG_BUF_SIZE 1024 // Size of the generic temp buffer for building message strings
#define APP_DISCONNECT_MSG "one of the clients disconnected" // Msg broadcasted on disconnect
#define APP_CONNECT_MSG_BUF_SIZE 64                          // New connection buffer size
#define APP_CONNECT_MSG " connected to the server"           // Msg broadcasted on new connection
#define APP_WELCOME_MSG "Welcome to PiHub!"                  // Msg sent to all new clients on connection

#define APP_GENERIC_FAILURE_MSG "generic system failure"
#define APP_CMD_INCOMPLETE_MSG "command incomplete"
#define APP_CMD_ERR_MSG "command not found (incorrect cmd / buffer empty or overflow / token too long)"


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
    Bme280_t sens_1;
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
        strcat(tmp_buf, APP_PIHUB_ERROR_MSG);
    } else {
        strcat(tmp_buf, APP_PIHUB_INFO_MSG);
    }

    // Concatenate the actual message and add a new line character
    strcat(tmp_buf, buf);
    strcat(tmp_buf, "\n");

    ServerError_t err_s = server_write(&app_ctx.server, *client, tmp_buf, strlen(tmp_buf));
    if(err_s != SERVER_ERR_OK) {
        log_error("server_write failed (ret: %d)", err_s);
    }
}

// Generic function for broadcasting PiHub messages to all clients (buf has to be a NULL terminated string no longer than APP_TEMP_MSG_BUF_SIZE!)
void app_broadcast(const char* buf, AppMsgType_t type) {
    char tmp_buf[APP_TEMP_MSG_BUF_SIZE] = "";

    // Start the message with PiHub msg type
    if(type == APP_MSG_TYPE_ERROR) {
        strcat(tmp_buf, APP_PIHUB_ERROR_MSG);
    } else {
        strcat(tmp_buf, APP_PIHUB_INFO_MSG);
    }

    // Concatenate the actual message and add a new line character
    strcat(tmp_buf, buf);
    strcat(tmp_buf, "\n");

    ServerError_t err_s = server_broadcast(&app_ctx.server, tmp_buf, strlen(tmp_buf));
    if(err_s != SERVER_ERR_OK) {
        log_error("server_write failed (ret: %d)", err_s);
    }
}

/************* Event handlers for Dispatcher *************/

void handle_gpio_set(char* argv, uint32_t argc, const void* cmd_ctx) {
    log_info("handle_gpio_set called");

    // The cmd context carries details about the client that invoked the command
    ServerClient_t* client = (ServerClient_t*)cmd_ctx;

    for(uint32_t arg = 0; (arg < argc) && (arg < DISPATCHER_MAX_ARGS); arg++) {
        if(strnlen(argv + (arg * DISPATCHER_ARG_MAX_SIZE), DISPATCHER_ARG_MAX_SIZE) < DISPATCHER_ARG_MAX_SIZE) {
            log_info("  arg %d: %s", arg, argv + (arg * DISPATCHER_ARG_MAX_SIZE));
        }
    }

    // @TODO: Add logic for setting GPIO
}

void handle_gpio_get(char* argv, uint32_t argc, const void* cmd_ctx) {
    log_info("handle_gpio_get called");
}

void handle_sensor_list(char* argv, uint32_t argc, const void* cmd_ctx) {
    log_info("handle_sensor_list called");
}

void handle_sensor_get(char* argv, uint32_t argc, const void* cmd_ctx) {
    log_info("handle_sensor_get called");
}

void handle_server_status(char* argv, uint32_t argc, const void* cmd_ctx) {
    log_debug("handle_server_status called");

    if(!cmd_ctx) {
        log_error("NULL context provided to the handle_server_status");
        return;
    }

    // The cmd context carries details about the client that invoked the command
    ServerClient_t* client = (ServerClient_t*)cmd_ctx;

    SysstatMemInfo_t mem_stats;
    SysstatNetInfo_t net_stats;
    SysstatUptimeInfo_t time_stats;

    SysstatError_t err_stat = sysstat_get_mem_info(&mem_stats);
    if(err_stat != SYSSTAT_ERR_OK) {
        log_error("sysstat_get_mem_info failed (ret: %d)", err_stat);
    }

    err_stat = sysstat_get_net_info(NET_INTERFACE_NAME, &net_stats);
    if(err_stat != SYSSTAT_ERR_OK) {
        log_error("sysstat_get_net_info failed (ret: %d)", err_stat);
    }

    err_stat = sysstat_get_uptime_info(&time_stats);
    if(err_stat != SYSSTAT_ERR_OK) {
        log_error("sysstat_get_uptime_info failed (ret: %d)", err_stat);
    }

    // Check the number of connected clients
    uint32_t clients_count = 0;
    for(ListNode_t* node = server_get_clients(&app_ctx.server); node != NULL; node = node->next) {
        clients_count++;
    }

    char buf[APP_TEMP_MSG_BUF_SIZE] = "";
    sprintf(buf, "Mem %lu kB/%lu kB (available/total) | Net tx: %lu kB, rx: %lu kB | Uptime %u.%hu s",
    mem_stats.available_kB, mem_stats.total_kB, net_stats.tx_bytes / 1000, net_stats.rx_bytes / 1000,
    time_stats.up.s, time_stats.up.ms);
    app_send_to_client(client, buf, APP_MSG_TYPE_INFO);

    sprintf(buf, "connected clients: %u", clients_count);
    app_send_to_client(client, buf, APP_MSG_TYPE_INFO);
}

void handle_server_uptime(char* argv, uint32_t argc, const void* cmd_ctx) {
    log_debug("handle_server_uptime called");

    if(!cmd_ctx) {
        log_error("NULL context provided to the handle_server_uptime");
        return;
    }

    // The cmd context carries details about the client that invoked the command
    ServerClient_t* client = (ServerClient_t*)cmd_ctx;

    SysstatUptimeInfo_t time_stats;

    SysstatError_t err_stat = sysstat_get_uptime_info(&time_stats);
    if(err_stat != SYSSTAT_ERR_OK) {
        log_error("sysstat_get_uptime_info failed (ret: %d)", err_stat);
    }

    // Check the number of connected clients
    uint32_t clients_count = 0;
    for(ListNode_t* node = server_get_clients(&app_ctx.server); node != NULL; node = node->next) {
        clients_count++;
    }

    char buf[APP_TEMP_MSG_BUF_SIZE] = "";
    sprintf(buf, "uptime %u.%hu s", time_stats.up.s, time_stats.up.ms);
    app_send_to_client(client, buf, APP_MSG_TYPE_INFO);
}

void handle_server_net(char* argv, uint32_t argc, const void* cmd_ctx) {
    log_debug("handle_server_net called");

    if(!cmd_ctx) {
        log_error("NULL context provided to the handle_server_net");
        return;
    }

    // The cmd context carries details about the client that invoked the command
    ServerClient_t* client = (ServerClient_t*)cmd_ctx;

    SysstatNetInfo_t net_stats;

    SysstatError_t err_stat = sysstat_get_net_info(NET_INTERFACE_NAME, &net_stats);
    if(err_stat != SYSSTAT_ERR_OK) {
        log_error("sysstat_get_net_info failed (ret: %d)", err_stat);
    }

    char buf[APP_TEMP_MSG_BUF_SIZE] = "";
    sprintf(buf, "net tx: %lu kB (%lu packets), rx: %lu kB (%lu packets)", net_stats.tx_bytes / 1000,
    net_stats.rx_packets, net_stats.rx_bytes / 1000, net_stats.tx_packets);
    app_send_to_client(client, buf, APP_MSG_TYPE_INFO);
}

void handle_server_shutdown(char* argv, uint32_t argc, const void* cmd_ctx) {
    log_debug("handle_server_shutdown called");
}

void handle_server_disconnect(char* argv, uint32_t argc, const void* cmd_ctx) {
    log_debug("handle_client_disconnect called");

    if(!cmd_ctx) {
        log_error("NULL context provided to the handle_server_net");
        return;
    }

    // The cmd context carries details about the client that invoked the command
    ServerClient_t* client = (ServerClient_t*)cmd_ctx;

    app_send_to_client(client, "disconnecting from the server...", APP_MSG_TYPE_INFO);

    ServerError_t err_s = server_disconnect(&app_ctx.server, *client);
    if(err_s != SERVER_ERR_OK) {
        log_error("server_disconnect failed (ret: %d)", err_s);
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

    ServerError_t err_s = server_broadcast(_ctx, APP_DISCONNECT_MSG, strlen(APP_DISCONNECT_MSG));
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

    // Give the app some margin before reinit is done
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

    const DispatcherCommandDef_t cmd_list[] = {
        // List of all commands to be supported
        { .target = "gpio", .action = "set", .callback_ptr = handle_gpio_set },
        { .target = "gpio", .action = "get", .callback_ptr = handle_gpio_get },
        { .target = "sensor", .action = "list", .callback_ptr = handle_sensor_list },
        { .target = "sensor", .action = "get", .callback_ptr = handle_sensor_get },
        { .target = "server", .action = "status", .callback_ptr = handle_server_status },
        { .target = "server", .action = "uptime", .callback_ptr = handle_server_uptime },
        { .target = "server", .action = "net", .callback_ptr = handle_server_net },
        { .target = "server", .action = "shutdown", .callback_ptr = handle_server_shutdown },
        { .target = "server", .action = "disconnect", .callback_ptr = handle_server_disconnect },

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
    AppError_t err = app_init_server();
    if(err != APP_ERR_OK) {
        return err;
    }

    // Initialize the dispatcher
    err = app_init_dispatcher();
    if(err != APP_ERR_OK) {
        return err;
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