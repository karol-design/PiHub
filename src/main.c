#include <string.h> // For: strlen()
#include <unistd.h> // For: sleep()

#include "app/parser.h"
#include "comm/network.h"
#include "utils/log.h"

void test_server();
void test_parser();
void test_ll();

int main() {
    test_server();
    // test_parser();
    // test_ll();
    return 0;
}

/************* Event handlers for Server *************/

void handle_client_connect(void* ctx, const ServerClient_t client) {
    log_info("handle_client_connect called");

    Server_t* _ctx = (Server_t*)ctx;

    char ip_str[IPV4_ADDRSTR_LENGHT];
    ServerError_t err = _ctx->get_client_ip(client, ip_str);
    log_debug("server->get_client_ip called (ret: %d)", err);

    char msg_client[64] = "Welcome ";
    strlcat(msg_client, ip_str, 64);
    strlcat(msg_client, "!\n", 64);
    err = _ctx->write(_ctx, client, msg_client, strlen(msg_client));
    log_debug("server->write called (ret: %d)", err);

    char msg_broadcast[64] = "New client (";
    strlcat(msg_broadcast, ip_str, 64);
    strlcat(msg_broadcast, ") joined the session!\n", 64);
    err = _ctx->broadcast(_ctx, msg_broadcast, strlen(msg_broadcast));
    log_debug("server->broadcast called (ret: %d)", err);
}

void handle_data_received(void* ctx, const ServerClient_t client) {
    log_info("handle_data_received called");

    Server_t* _ctx = (Server_t*)ctx;
    char buf[100];
    ssize_t len;
    ServerError_t err = _ctx->read(ctx, client, buf, 100, &len);
    log_debug("server->read called (ret: %d)", err);
}

void handle_client_disconnect(void* ctx, const ServerClient_t client) {
    log_info("handle_client_disconnect called");

    Server_t* _ctx = (Server_t*)ctx;

    const char* msg_broadcast = "One of the clients disconnected from the session!\n";
    ServerError_t err = _ctx->broadcast(_ctx, msg_broadcast, strlen(msg_broadcast));
    log_debug("server->broadcast called (ret: %d)", err);
}

void handle_server_failure(void* ctx, const ServerError_t err) {
    Server_t* _ctx = (Server_t*)ctx;
    log_info("handle_server_failure called with error: %d", err);
    log_info("attempting to restart the server");

    // Shut down the server...
    ServerError_t ret = _ctx->shutdown(_ctx);
    log_debug("server->shutdown called (ret: %d)", err);

    // ...and restart it
    ret = _ctx->run(_ctx);
    log_debug("server->run called (ret: %d)", err);
}

/************* Event handlers for Parser *************/
void handle_gpio_set(char* argv, uint32_t argc) {
    log_info("handle_gpio_set called");

    for(uint32_t arg = 0; (arg < argc) && (arg < PARSER_MAX_ARGS); arg++) {
        if(strnlen(argv + (arg * PARSER_ARG_MAX_SIZE), PARSER_ARG_MAX_SIZE) < PARSER_ARG_MAX_SIZE) {
            log_info("  arg %d: %s", arg, argv + (arg * PARSER_ARG_MAX_SIZE));
        }
    }
}

void handle_gpio_toggle(char* argv, uint32_t argc) {
    log_info("handle_gpio_toggle called");

    for(uint32_t arg = 0; (arg < argc) && (arg < PARSER_MAX_ARGS); arg++) {
        if(strnlen(argv + (arg * PARSER_ARG_MAX_SIZE), PARSER_ARG_MAX_SIZE) < PARSER_ARG_MAX_SIZE) {
            log_info("  arg %d: %s", arg, argv + (arg * PARSER_ARG_MAX_SIZE));
        }
    }
}

void handle_sensor_list(char* argv, uint32_t argc) {
    log_info("handle_sensor_list called");

    for(uint32_t arg = 0; (arg < argc) && (arg < PARSER_MAX_ARGS); arg++) {
        if(strnlen(argv + (arg * PARSER_ARG_MAX_SIZE), PARSER_ARG_MAX_SIZE) < PARSER_ARG_MAX_SIZE) {
            log_info("  arg %d: %s", arg, argv + (arg * PARSER_ARG_MAX_SIZE));
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

void test_parser() {
    Parser_t parser;
    ParserConfig_t cfg = { .delim = " " };
    ParserCommandDef_t gpio_set_cmd = { .target = "gpio", .action = "set", .callback_ptr = handle_gpio_set };
    ParserCommandDef_t gpio_toggle_cmd = { .target = "gpio", .action = "toggle", .callback_ptr = handle_gpio_toggle };
    ParserCommandDef_t sensor_list_cmd = { .target = "sensor", .action = "list", .callback_ptr = handle_sensor_list };

    ParserError_t err = parser_init(&parser, cfg);
    log_debug("parser_init called (ret %d)", err);

    err = parser.add_cmd(&parser, 0, gpio_set_cmd);
    log_debug("parser.add_cmd called (ret %d)", err);

    // Test too many args
    err = parser.execute(&parser, "gpio set 1 2 3 4 5 6 7 8 9 10 11");
    log_debug("parser.execute called (ret %d)", err);

    // Test empty buffer
    err = parser.execute(&parser, " ");
    log_debug("parser.execute called (ret %d)", err);

    // Test NULL ptr
    err = parser.execute(&parser, NULL);
    log_debug("parser.execute called (ret %d)", err);

    // Test case-sensitivity
    err = parser.execute(&parser, "GPiO SeT 0 OK");
    log_debug("parser.execute called (ret %d)", err);

    // Test unknown command
    err = parser.execute(&parser, "GPiO SeTs 0");
    log_debug("parser.execute called (ret %d)", err);

    /* ------------------ */

    err = parser.add_cmd(&parser, 1, gpio_toggle_cmd);
    log_debug("parser.add_cmd called (ret %d)", err);

    // Test case-sensitivity
    err = parser.execute(&parser, "GPiO toGGle PIN1 OK");
    log_debug("parser.execute called (ret %d)", err);

    /* ------------------ */

    err = parser.add_cmd(&parser, 2, sensor_list_cmd);
    log_debug("parser.add_cmd called (ret %d)", err);

    // Test case-sensitivity
    err = parser.execute(&parser, "Sensor LIST S1 OK");
    log_debug("parser.execute called (ret %d)", err);

    // Test case-sensitivity
    err = parser.execute(&parser, "GPiO toGGle PIN1 OK");
    log_debug("parser.execute called (ret %d)", err);

    /* ------------------ */

    err = parser.remove_cmd(&parser, 1);
    log_debug("parser.remove_cmd called (ret %d)", err);

    // Test case-sensitivity
    err = parser.execute(&parser, "GPiO toGGle PIN1");
    log_debug("parser.execute called (ret %d)", err);

    parser.deinit(&parser);
}

void test_server() {
    ServerCallbackList_t server_cb_list = { .on_client_connect = handle_client_connect,
        .on_client_disconnect = handle_client_disconnect,
        .on_data_received = handle_data_received,
        .on_server_failure = handle_server_failure };

    ServerConfig_t server_cfg = { .port = "65002", .cb_list = server_cb_list, .max_clients = 4, .max_conn_requests = 10 };

    Server_t* server = server_create(server_cfg);
    if(!server) {
        log_error("server_create failed");
        return;
    }
    log_debug("server_create called");

    // Test: running a server
    ServerError_t err = server->run(server);
    log_debug("server->run called (ret: %d)", err);

    sleep(40);

    // Test: printing all clients and their IP addresses
    log_info("Connected clients:");
    ListNode_t* node = server->get_clients(server);
    while(node) {
        char ip_str[IPV4_ADDRSTR_LENGHT];
        ServerClient_t* client = (ServerClient_t*)(node->data);
        server->get_client_ip(*client, ip_str);
        log_info("Client fd: %d, thread id: %lu, ip: %s", client->fd, client->thread, ip_str);
        node = node->next;
    }

    node = server->get_clients(server);
    ServerClient_t* client = (ServerClient_t*)(node->data);

    // Test: disconnecting a specific client (in this example: ll head)
    const char* msg_client = "You are disconnected!!\n";
    err = server->write(server, *client, msg_client, strlen(msg_client));
    log_debug("server->write called (ret: %d)", err);
    err = server->disconnect(server, *client, false);
    log_debug("server->disconnect called (ret: %d)", err);

    sleep(20);

    // Test: shutting down the server
    err = server->shutdown(server);
    log_info("server->shutdown called (ret: %d)", err);

    // Test: destroying the server instance
    err = server->destroy(&server);
    log_info("server->destroy called (ret: %d)", err);

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
