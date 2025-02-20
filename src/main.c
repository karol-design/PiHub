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

void handle_client_connect(void* ctx, void* this, ServerClient_t client) {
    (void*)ctx; // Argument not used
    log_info("handle_client_connect called");

    Server_t* _this = (Server_t*)this;

    const char* msg_client = "Welcome!\n";
    ServerError_t err = _this->write(_this, client, msg_client, strlen(msg_client));
    log_info("server->write called (ret: %d)", err);

    const char* msg_broadcast = "New client joined the session!\n";
    err = _this->broadcast(_this, msg_broadcast, strlen(msg_broadcast));
    log_info("server->broadcast called (ret: %d)", err);
}

void handle_data_received(void* ctx, void* this, ServerClient_t client) {
    (void*)ctx; // Argument not used
    log_info("handle_data_received called");

    Server_t* _this = (Server_t*)this;
    char buf[100];
    ssize_t len;
    ServerError_t err = _this->read(this, client, buf, 100, &len);
    log_info("server->read called (ret: %d)", err);
}

void handle_client_disconnect(void* ctx, void* this, ServerClient_t client) {
    (void*)ctx; // Argument not used
    log_info("handle_client_disconnect called");

    Server_t* _this = (Server_t*)this;

    const char* msg_broadcast = "One of the clients disconnected from the session!\n";
    ServerError_t err = _this->broadcast(_this, msg_broadcast, strlen(msg_broadcast));
    log_info("server->broadcast called (ret: %d)", err);
}

void handle_server_shutdown(void* ctx, void* this) {
    (void*)ctx; // Argument not used
    log_info("handle_server_shutdown called");
}

/************* Functional tests *************/

void test_server() {
    ServerCallbackList_t server_cb_list = { .on_client_connect = handle_client_connect,
        .on_client_disconnect = handle_client_disconnect,
        .on_data_received = handle_data_received,
        .on_server_shutdown = handle_server_shutdown };

    ServerConfig_t server_cfg = { .port = "65002", .cb_list = server_cb_list, .max_clients = 10, .max_conn_requestes = 10 };

    Server_t* server = server_create(&server_cfg);
    if(!server) {
        log_error("server_create failed");
        return;
    }
    log_info("server_create called");

    // Test: running a server
    ServerError_t err = server->run(server);
    log_info("server->run called (ret: %d)", err);

    sleep(20);

    // Test: printing all clients
    log_info("Connected clients:");
    ListNode_t* node = server->clients_list->head;
    while(node) {
        char ip_str[IPV4_ADDRSTR_LENGHT];
        ServerClient_t* client = (ServerClient_t*)(node->data);
        server->get_client_ip(server, *client, ip_str);
        log_info("Client fd: %d, thread id: %lu, ip: %s", client->fd, client->thread, ip_str);
        node = node->next;
    }

    node = server->clients_list->head;
    ServerClient_t* client = (ServerClient_t*)(node->data);

    // Test: disconnecting a specific client (in this example: ll head)
    const char* msg_client = "You are disconnected!!\n";
    err = server->write(server, *client, msg_client, strlen(msg_client));
    log_info("server->write called (ret: %d)", err);
    err = server->disconnect(server, *client);
    log_info("server->disconnect called (ret: %d)", err);

    sleep(20);

    // Test: shutting down the server
    err = server->shutdown(server);
    log_info("server->shutdown called (ret: %d)", err);

    // Test: destroying the server instance
    err = server->destroy(&server);
    log_info("server->destroy called (ret: %d)", err);

    sleep(5);
}
