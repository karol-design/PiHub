#include <string.h> // For: strlen()
#include <unistd.h> // For: sleep()

#include "app/dispatcher.h"
#include "comm/network.h"

#define LOGS_ENABLED 1
#include "utils/log.h"

void test_server();
void test_dispatcher();
void test_ll();

int main() {
    // test_dispatcher();
    // test_ll();
    test_server();
    return 0;
}

/************* Event handlers for Server *************/

void handle_client_connect(void* ctx, const ServerClient_t client) {
    log_info("handle_client_connect called");

    Server_t* _ctx = (Server_t*)ctx;

    char ip_str[IPV4_ADDRSTR_LENGHT];
    ServerError_t err = server_get_client_ip(client, ip_str);
    log_debug("server_get_client_ip called (ret: %d)", err);

    char msg_client[64] = "Welcome ";
    strlcat(msg_client, ip_str, 64);
    strlcat(msg_client, "!\n", 64);
    err = server_write(_ctx, client, msg_client, strlen(msg_client));
    log_debug("server_write called (ret: %d)", err);

    char msg_broadcast[64] = "New client (";
    strlcat(msg_broadcast, ip_str, 64);
    strlcat(msg_broadcast, ") joined the session!\n", 64);
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
        char ip_str[IPV4_ADDRSTR_LENGHT];
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
