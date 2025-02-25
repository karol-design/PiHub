#include <string.h> // For: strlen()
#include <unistd.h> // For: sleep()

#include "comm/network.h"
#include "utils/log.h"

void test_server();

int main() {
    test_server();
    return 0;
}

/************* Event handlers for Server *************/

void handle_client_connect(const void* ctx, const ServerClient_t client) {
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

void handle_data_received(const void* ctx, const ServerClient_t client) {
    log_info("handle_data_received called");

    Server_t* _ctx = (Server_t*)ctx;
    char buf[100];
    ssize_t len;
    ServerError_t err = _ctx->read(ctx, client, buf, 100, &len);
    log_debug("server->read called (ret: %d)", err);
}

void handle_client_disconnect(const void* ctx, const ServerClient_t client) {
    log_info("handle_client_disconnect called");

    Server_t* _ctx = (Server_t*)ctx;

    const char* msg_broadcast = "One of the clients disconnected from the session!\n";
    ServerError_t err = _ctx->broadcast(_ctx, msg_broadcast, strlen(msg_broadcast));
    log_debug("server->broadcast called (ret: %d)", err);
}

void handle_server_failure(const void* ctx, const ServerError_t err) {
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

/************* Functional tests *************/

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
