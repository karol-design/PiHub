#include <string.h> // For: strlen()
#include <unistd.h> // For: sleep()

#include "app/dispatcher.h"
#include "comm/network.h"
#include "hw/gpio.h"
#include "hw/hw_interface.h"
#include "hw/i2c_bus.h"
#include "sensors/bme280.h"
#include "sensors/bme280_regs.h"
#include "sensors/sensor.h"
#include "utils/list.h"

#define LOGS_ENABLED 1
#include "utils/log.h"

void test_server();
void test_dispatcher();
void test_ll();
void test_i2c_bus();
void test_bme280();
void test_gpio();

int main() {
    // test_dispatcher();
    // test_ll();
    // test_server();
    // test_i2c_bus();
    test_bme280();
    test_gpio();
    return 0;
}

/************* Event handlers for Server *************/

void handle_client_connect(void* ctx, const ServerClient_t client) {
    log_info("handle_client_connect called");

    Server_t* _ctx = (Server_t*)ctx;

    char ip_str[IPV4_ADDRSTR_LENGTH];
    ServerError_t err = server_get_client_ip(client, ip_str);
    log_debug("server_get_client_ip called (ret: %d)", err);

    char msg_client[64] = "Welcome ";
    strncat(msg_client, ip_str, 63);
    strncat(msg_client, "!\n", 63);
    err = server_write(_ctx, client, msg_client, strlen(msg_client));
    log_debug("server_write called (ret: %d)", err);

    char msg_broadcast[64] = "New client (";
    strncat(msg_broadcast, ip_str, 63);
    strncat(msg_broadcast, ") joined the session!\n", 63);
    err = server_broadcast(_ctx, msg_broadcast, strlen(msg_broadcast));
    log_debug("server_broadcast called (ret: %d)", err);
}

void handle_data_received(void* ctx, const ServerClient_t client) {
    log_info("handle_data_received called");

    Server_t* _ctx = (Server_t*)ctx;
    char buf[100];
    ssize_t len;
    ServerError_t err = server_read(ctx, client, buf, 100, &len);
    log_debug("server_read called (ret: %d)", err);
}

void handle_client_disconnect(void* ctx, const ServerClient_t client) {
    log_info("handle_client_disconnect called");

    Server_t* _ctx = (Server_t*)ctx;

    const char* msg_broadcast = "One of the clients disconnected from the session!\n";
    ServerError_t err = server_broadcast(_ctx, msg_broadcast, strlen(msg_broadcast));
    log_debug("server_broadcast called (ret: %d)", err);
}

void handle_server_failure(void* ctx, const ServerError_t err) {
    Server_t* _ctx = (Server_t*)ctx;
    log_info("handle_server_failure called with error: %d", err);
    log_info("attempting to restart the server");

    // Shut down the server_..
    ServerError_t ret = server_shutdown(_ctx);
    log_debug("server_shutdown called (ret: %d)", err);

    // ...and restart it
    ret = server_run(_ctx);
    log_debug("server_run called (ret: %d)", err);
}

/************* Event handlers for Dispatcher *************/

void handle_gpio_set(char* argv, uint32_t argc) {
    log_info("handle_gpio_set called");

    for(uint32_t arg = 0; (arg < argc) && (arg < DISPATCHER_MAX_ARGS); arg++) {
        if(strnlen(argv + (arg * DISPATCHER_ARG_MAX_SIZE), DISPATCHER_ARG_MAX_SIZE) < DISPATCHER_ARG_MAX_SIZE) {
            log_info("  arg %d: %s", arg, argv + (arg * DISPATCHER_ARG_MAX_SIZE));
        }
    }
}

void handle_gpio_toggle(char* argv, uint32_t argc) {
    log_info("handle_gpio_toggle called");

    for(uint32_t arg = 0; (arg < argc) && (arg < DISPATCHER_MAX_ARGS); arg++) {
        if(strnlen(argv + (arg * DISPATCHER_ARG_MAX_SIZE), DISPATCHER_ARG_MAX_SIZE) < DISPATCHER_ARG_MAX_SIZE) {
            log_info("  arg %d: %s", arg, argv + (arg * DISPATCHER_ARG_MAX_SIZE));
        }
    }
}

void handle_sensor_list(char* argv, uint32_t argc) {
    log_info("handle_sensor_list called");

    for(uint32_t arg = 0; (arg < argc) && (arg < DISPATCHER_MAX_ARGS); arg++) {
        if(strnlen(argv + (arg * DISPATCHER_ARG_MAX_SIZE), DISPATCHER_ARG_MAX_SIZE) < DISPATCHER_ARG_MAX_SIZE) {
            log_info("  arg %d: %s", arg, argv + (arg * DISPATCHER_ARG_MAX_SIZE));
        }
    }
}

/************* Compare and print callbacks for List *************/

int compare_data(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

ListError_t print_node(void* data) {
    printf("%d ", *(int*)data);
    return LIST_ERR_OK;
}

/************* Functional tests *************/

void test_dispatcher() {
    Dispatcher_t dispatcher;
    DispatcherConfig_t cfg = { .delim = " " };
    DispatcherCommandDef_t gpio_set_cmd = { .target = "gpio", .action = "set", .callback_ptr = handle_gpio_set };
    DispatcherCommandDef_t gpio_toggle_cmd = { .target = "gpio", .action = "toggle", .callback_ptr = handle_gpio_toggle };
    DispatcherCommandDef_t sensor_list_cmd = { .target = "sensor", .action = "list", .callback_ptr = handle_sensor_list };

    DispatcherError_t err = dispatcher_init(&dispatcher, cfg);
    log_info("dispatcher_init called (ret %d)", err);

    err = dispatcher_register(&dispatcher, 0, gpio_set_cmd);
    log_info("dispatcher_register called (ret %d)", err);

    // Test too many args
    err = dispatcher_execute(&dispatcher, "gpio set 1 2 3 4 5 6 7 8 9 10 11");
    log_info("dispatcher_execute called (ret %d)", err);

    // Test empty buffer
    err = dispatcher_execute(&dispatcher, " ");
    log_info("dispatcher_execute called (ret %d)", err);

    // Test NULL ptr
    err = dispatcher_execute(&dispatcher, NULL);
    log_info("dispatcher_execute called (ret %d)", err);

    // Test case-sensitivity
    err = dispatcher_execute(&dispatcher, "GPiO SeT 0 OK");
    log_info("dispatcher_execute called (ret %d)", err);

    // Test unknown command
    err = dispatcher_execute(&dispatcher, "GPiO SeTs 0");
    log_info("dispatcher_execute called (ret %d)", err);

    /* ------------------ */

    err = dispatcher_register(&dispatcher, 1, gpio_toggle_cmd);
    log_info("dispatcher_register called (ret %d)", err);

    // Test case-sensitivity
    err = dispatcher_execute(&dispatcher, "GPiO toGGle PIN1 OK");
    log_info("dispatcher_execute called (ret %d)", err);

    /* ------------------ */

    err = dispatcher_register(&dispatcher, 2, sensor_list_cmd);
    log_info("dispatcher_register called (ret %d)", err);

    // Test case-sensitivity
    err = dispatcher_execute(&dispatcher, "Sensor LIST S1 OK");
    log_info("dispatcher_execute called (ret %d)", err);

    // Test case-sensitivity
    err = dispatcher_execute(&dispatcher, "GPiO toGGle PIN1 OK");
    log_info("dispatcher_execute called (ret %d)", err);

    /* ------------------ */

    err = dispatcher_deregister(&dispatcher, 1);
    log_info("dispatcher_deregister called (ret %d)", err);

    // Test case-sensitivity
    err = dispatcher_execute(&dispatcher, "GPiO toGGle PIN1");
    log_info("dispatcher_execute called (ret %d)", err);

    err = dispatcher_deinit(&dispatcher);
    log_info("dispatcher_deinit called (ret %d)", err);
}

void test_server() {
    ServerCallbackList_t server_cb_list = { .on_client_connect = handle_client_connect,
        .on_client_disconnect = handle_client_disconnect,
        .on_data_received = handle_data_received,
        .on_server_failure = handle_server_failure };

    ServerConfig_t server_cfg = { .port = "65002", .cb_list = server_cb_list, .max_clients = 4, .max_conn_requests = 10 };

    Server_t server;
    ServerError_t err = server_init(&server, server_cfg);
    log_debug("server_init called (ret: %d)", err);

    // Test: running a server
    err = server_run(&server);
    log_debug("server_run called (ret: %d)", err);

    sleep(40);

    // Test: printing all clients and their IP addresses
    log_info("Connected clients:");
    ListNode_t* node = server_get_clients(&server);
    while(node) {
        char ip_str[IPV4_ADDRSTR_LENGTH];
        ServerClient_t* client = (ServerClient_t*)(node->data);
        server_get_client_ip(*client, ip_str);
        log_info("Client fd: %d, thread id: %lu, ip: %s", client->fd, client->thread, ip_str);
        node = node->next;
    }

    node = server_get_clients(&server);
    if(node) {
        ServerClient_t* client = (ServerClient_t*)(node->data);

        // Test: disconnecting a specific client (in this example: ll head)
        const char* msg_client = "You are disconnected!!\n";
        err = server_write(&server, *client, msg_client, strlen(msg_client));
        log_debug("server_write called (ret: %d)", err);
        err = server_disconnect(&server, *client, false);
        log_debug("server_disconnect called (ret: %d)", err);
    }

    sleep(20);

    // Test: shutting down the server
    err = server_shutdown(&server);
    log_info("server_shutdown called (ret: %d)", err);

    // Test: deinit the server instance
    err = server_deinit(&server);
    log_info("server_deinit called (ret: %d)", err);

    sleep(5);
}

void test_ll() {
    int data = 12;
    int *tmp, *data_ptr = &data;

    // Test creating the ll and ops on empty list
    List_t ll;
    log_info("llist_init called and returned %d", llist_init(&ll, compare_data));

    log_info("traverse (print) called and returned %d", ll.traverse(&ll, print_node));
    log_info("get_length called and returned %d", (int)ll.get_length(&ll));
    if(ll.get_head(&ll) != NULL) {
        log_info("get_head called and returned %d", *(int*)(ll.get_head(&ll)->data));
        log_info("get_tail called and returned %d", *(int*)(ll.get_tail(&ll)->data));
    }
    log_info("--------");

    // Test adding a node and operating on one node
    log_info("push called and returned %d", ll.push(&ll, (void*)data_ptr, sizeof(int)));

    log_info("traverse (print) called and returned %d", ll.traverse(&ll, print_node));
    log_info("get_length called and returned %d", (int)ll.get_length(&ll));
    if(ll.get_head(&ll) != NULL) {
        log_info("get_head called and returned %d", *(int*)(ll.get_head(&ll)->data));
        log_info("get_tail called and returned %d", *(int*)(ll.get_tail(&ll)->data));
    }
    log_info("--------");

    // Test adding a second and third node
    *data_ptr = 15;
    log_info("push called and returned %d", ll.push(&ll, (void*)data_ptr, sizeof(int)));
    *data_ptr = -12;
    log_info("push called and returned %d", ll.push(&ll, (void*)data_ptr, sizeof(int)));

    log_info("traverse (print) called and returned %d", ll.traverse(&ll, print_node));
    log_info("get_length called and returned %d", (int)ll.get_length(&ll));
    if(ll.get_head(&ll) != NULL) {
        log_info("get_head called and returned %d", *(int*)(ll.get_head(&ll)->data));
        log_info("get_tail called and returned %d", *(int*)(ll.get_tail(&ll)->data));
    }
    log_info("--------");

    // Test removing a node
    *data_ptr = 15;
    log_info("remove called and returned %d", ll.remove(&ll, (void*)data_ptr));

    log_info("traverse (print) called and returned %d", ll.traverse(&ll, print_node));
    log_info("get_length called and returned %d", (int)ll.get_length(&ll));
    if(ll.get_head(&ll) != NULL) {
        log_info("get_head called and returned %d", *(int*)(ll.get_head(&ll)->data));
        log_info("get_tail called and returned %d", *(int*)(ll.get_tail(&ll)->data));
    }
    log_info("--------");

    // Test destroying a node
    log_info("destroy called and returned %d", ll.deinit(&ll));
    log_info("--------");

    return;
}

void test_i2c_bus() {
#define BME280_ADDR 0x76 // Address of the sensor (0x76 for BME280 or 0x5D for LPS25H)

    I2CBus_t i2c;
    I2CBusConfig_t cfg = {
        .i2c_adapter = 1, // On RPI the I2C adapter is mounted as '/dev/i2c-1'
    };
    I2CBusError_t err = i2c_bus_init(&i2c, cfg);
    log_info("i2c_bus_init called and returned %d", err);
    if(err != I2C_BUS_ERR_OK) {
        return;
    }

    // Set: max oversampling (x16) on temp and press measurements; Normal mode
    uint8_t write_buf = 0xFF;

    i2c_bus_write(&i2c, BME280_ADDR, BME280_REG_CTRL_MEAS, &write_buf, sizeof(write_buf));

    // Set: max standby (20ms); filter off; 3-wire SPI off
    write_buf = 0x70;
    i2c_bus_write(&i2c, BME280_ADDR, BME280_REG_CONFIG, &write_buf, sizeof(write_buf));

    // Check sensor ID (should be: 0xBD for LPS25H and 0x60 for BM280)
    uint8_t id_buf;
    i2c_bus_read(&i2c, BME280_ADDR, BME280_REG_ID, &id_buf, sizeof(id_buf));
    log_info("sensor id: %02X", id_buf);

    /* Data readout is done by starting a burst read from 0xF7 to 0xFC (temperature and pressure) or
     * from 0xF7 to 0xFE (temperature, pressure and humidity). The data are read out in an unsigned
     * 20-bit format both for pressure and for temperature and in an unsigned 16-bit format for
     * humidity.*/
    uint8_t temp_buf[3];
    i2c_bus_read(&i2c, BME280_ADDR, BME280_REG_TEMP_LSB, temp_buf, sizeof(temp_buf));
    log_info("temperature: %hu | %hu | %hu", temp_buf[0], temp_buf[1], temp_buf[2]);

    err = i2c_bus_deinit(&i2c);
    log_info("i2c_bus_deinit called and returned %d", err);
}

void test_bme280() {
    HwInterface_t hw_interface_i2c;
    HwInterfaceError_t h_ret = hw_interface_init(&hw_interface_i2c, HW_INTERFACE_I2C);
    log_info("hw_interface_init called and returned %d", h_ret);

    Bme280_t s_bme280_1;
    SensorError_t s_ret = bme280_init(&s_bme280_1, 0x76, hw_interface_i2c);
    log_info("bme280_init called and returned %d", s_ret);

    float temp, hum, press;
    s_ret = bme280_get_temp(&s_bme280_1, &temp);
    log_info("bme280_get_temp called and returned %d (out: %.2f *C)", s_ret, temp);

    s_ret = bme280_get_hum(&s_bme280_1, &hum);
    log_info("bme280_get_hum called and returned %d (out: %.2f %%)", s_ret, hum);

    s_ret = bme280_get_press(&s_bme280_1, &press);
    log_info("bme280_get_press called and returned %d (out: %.2f Pa)", s_ret, press);

    s_ret = bme280_check_id(&s_bme280_1);
    log_info("bme280_check_id called and returned %d", s_ret);

    s_ret = bme280_deinit(&s_bme280_1);
    log_info("deinit called and returned %d", s_ret);

    h_ret = hw_interface_deinit(&hw_interface_i2c);
    log_info("hw_interface_deinit called and returned %d", h_ret);
}

void test_gpio() {
    Gpio_t ctx;
    log_info("gpio_init called and returned %d", gpio_init(&ctx));

    log_info("gpio_set (pin: 17, state: HIGH) called and returned %d", gpio_set(&ctx, 17, 1));
    log_info("gpio_set (pin: 27, state: LOW) called and returned %d", gpio_set(&ctx, 27, 0));
    log_info("gpio_set (pin: 22, state: HIGH) called and returned %d", gpio_set(&ctx, 22, 1));

    uint8_t state;
    GpioError_t err = gpio_get(&ctx, 5, &state);
    log_info("gpio_set (pin: 5, state: %hu) called and returned %d", state, err);

    err = gpio_get(&ctx, 6, &state);
    log_info("gpio_set (pin: 6, state: %hu) called and returned %d", state, err);

    log_info("gpio_deinit called and returned %d", gpio_deinit(&ctx));
}