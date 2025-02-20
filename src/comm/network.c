#include "comm/network.h"

#include <arpa/inet.h>   // For: inet_ntop
#include <errno.h>       // For: errno
#include <netdb.h>       // For: struct addrinfo, getaddrinfo(), socket()
#include <stdbool.h>     // For: bool
#include <string.h>      // For: memset, strerror
#include <sys/epoll.h>   // For: epoll*
#include <sys/eventfd.h> // Required for eventfd
#include <unistd.h>      // For: close

#include "utils/common.h"
#include "utils/log.h"

#define SERVER_IP_VER AF_INET       // IP version: IPv4
#define SERVER_SOCKTYPE SOCK_STREAM // UDP/TCP: TCP

typedef struct {
    Server_t* server;
    ServerClient_t client;
} ClientHandlerArgs_t;

// All functions are static; the user should use pointers from the structure
Server_t* server_create(const ServerConfig_t* cfg);
STATIC ServerError_t server_run(Server_t* this);
STATIC ServerError_t server_read(Server_t* this, ServerClient_t client, uint8_t* buf, size_t buf_length, ssize_t* len);
STATIC ServerError_t server_write(Server_t* this, ServerClient_t client, const uint8_t* data, size_t len);
STATIC ServerError_t server_broadcast(Server_t* this, const uint8_t* data, size_t len);
STATIC ServerError_t server_disconnect(Server_t* this, ServerClient_t client);
STATIC ServerError_t server_shutdown(Server_t* this);
STATIC ServerError_t server_destroy(Server_t** this);

/**
 * @brief Create and bind a network socket
 * @param[in]  port  Port for which a socket should be created (e.g. "65001")
 * @return Socket file descriptor if succesful, -1 otherwise
 */
STATIC int server_create_socket(const char* port);

/**
 * @brief Run an infinite loop that will monitor server's incomming connections [Thread]
 * @param[in]  arg  Pointer to the Server instance
 * @return This function should not return
 */
STATIC void* server_listen(void*);

/**
 * @brief Run an infinite loop that waits for user input and calls appropriate callbacks [Thread]
 * @param[in]  arg  Pointer to the Server instance
 * @return This function should not return
 */
STATIC void* server_client_handler(void*);

/**
 * @brief Callback for comparing two client file descriptors (required for Linked List)
 * @param[in]  a Pointer to the first ll data item
 * @param[in]  b Pointer to the second ll data item
 * @return Difference between a and b or 0 if they are equal
 */
STATIC int compare_client_fd(const void* a, const void* b);


STATIC ServerError_t server_run(Server_t* this) {
    if(!this) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    // Start accepting connection on the socket
    int ret = listen(this->fd, this->cfg.max_conn_requestes);
    if(ret == -1) {
        log_error("listen() returned: -1 (err: %s)", strerror(errno));
        return SERVER_ERR_NET_FAILURE;
    }
    log_info("server listening on port %s", this->cfg.port);

    // Create an eventfd for synchronization (shutdown request)
    this->shutdown_eventfd = eventfd(0, EFD_NONBLOCK);
    if(this->shutdown_eventfd == -1) {
        log_error("eventfd() failed (err: %s)", strerror(errno));
        pthread_exit(NULL);
    }

    // Create a listening thread
    pthread_t server_listening_thread;
    ret = pthread_create(&server_listening_thread, NULL, server_listen, (void*)this);
    if(ret != 0) {
        log_error("pthread_create() returned: %d", ret);
        return SERVER_ERR_PTHREAD_FAILURE;
    }
    ret = pthread_detach(server_listening_thread);
    if(ret != 0) {
        log_error("pthread_detach() returned: %d", ret);
        return SERVER_ERR_PTHREAD_FAILURE;
    }

    return SERVER_ERR_OK;
}

STATIC ServerError_t server_read(Server_t* this, ServerClient_t client, uint8_t* buf, size_t buf_length, ssize_t* len) {
    if(!this || !buf || !len) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    (*len) = recv(client.fd, (void*)buf, buf_length, 0);
    if(*len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        log_info("no data available to be read yet");
        return SERVER_ERR_OK; // No error, just wait for more data
    }

    if(*len <= 0) {
        log_info("client (fd: %d) disconnected from the server", client.fd);

        // Call "client disconnect" handler
        this->cfg.cb_list.on_client_disconnect(NULL, this, client);

        ServerError_t err = this->disconnect(this, client);
        if(err != SERVER_ERR_OK) {
            log_error("server_disconnect failed (returned: %d)", err);
        }

        pthread_exit(NULL); // Terminate this (client) thread (TODO: improve disconnect handling incl. garbage collection)
    }

    log_info("received %lu bytes from the client (fd: %d)", *len, client.fd);
    return SERVER_ERR_OK;
}

STATIC ServerError_t server_write(Server_t* this, ServerClient_t client, const uint8_t* data, size_t len) {
    if(!this || !data) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    ssize_t bytes = send(client.fd, (const void*)data, len, 0);
    if(bytes != len) {
        log_error("send() returned %zd instead of %lu", bytes, len);
        return SERVER_ERR_NET_FAILURE;
    }

    log_info("%lu bytes sent to the client (fd: %d)", len, client.fd);

    return SERVER_ERR_OK;
}

STATIC ServerError_t server_broadcast(Server_t* this, const uint8_t* data, size_t len) {
    if(!this || !data) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    ListNode_t* client = this->clients_list->get_head(this->clients_list);
    ServerClient_t client_fd;

    // Write data to all clients (if any)
    while(client) {
        client_fd = *(ServerClient_t*)(client->data);
        server_write(this, client_fd, data, len);
        client = client->next;
    }

    return SERVER_ERR_OK;
}

STATIC ServerError_t server_disconnect(Server_t* this, ServerClient_t client) {
    if(!this) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    uint64_t signal_value = 1;
    ssize_t ret = write(client.disconnect_eventfd, &signal_value, sizeof(signal_value));
    if(ret != sizeof(signal_value)) {
        log_error("failed to write to disconnect_eventfd (err: %s)", strerror(errno));
        return SERVER_ERR_GENERIC;
    }

    ListError_t err = this->clients_list->remove(this->clients_list, &client);
    if(err != LIST_ERR_OK) {
        log_error("failed to remove the client from the clients_list (err: %d)", err);
        return SERVER_ERR_GENERIC;
    }

    return SERVER_ERR_OK;
}

STATIC ServerError_t server_shutdown(Server_t* this) {
    if(!this) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    ListNode_t* node = this->clients_list->get_head(this->clients_list);
    ServerError_t err = SERVER_ERR_OK;
    ServerClient_t* client;

    // Disconnect all clients first
    while(node) {
        client = (ServerClient_t*)node->data;
        node = node->next;
        err = server_disconnect(this, *client);
        if(err != SERVER_ERR_OK) {
            return err;
        }
    }

    // Write to shutdown eventfd to stop server's listening thread
    uint64_t signal_value = 1;
    ssize_t ret = write(this->shutdown_eventfd, &signal_value, sizeof(signal_value));
    if(ret != sizeof(signal_value)) {
        log_error("failed to write to shutdown_eventfd (err: %s)", strerror(errno));
        return SERVER_ERR_GENERIC;
    }

    return SERVER_ERR_OK;
}

STATIC ServerError_t server_destroy(Server_t** this) {
    if(!this || !*this) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    // TODO: Add a check if server is not running

    ListError_t err = (*this)->clients_list->destroy(&((*this)->clients_list));
    if(err != LIST_ERR_OK) {
        log_error("failed to destroy linked list with client FDs (llist_destroy() returned: %d)", err);
        return SERVER_ERR_GENERIC;
    }

    // Free the memory allocated for this server struct instance
    free(*this);
    *this = NULL;

    return SERVER_ERR_OK;
}

STATIC void* server_listen(void* arg) {
    // TODO: Split server_listen into at least two functions (separation of concerns and shorter functions)
    if(!arg) {
        log_error("NULL ptr provided to server_listen - exiting the thread!");
        pthread_exit(NULL);
    }

    Server_t* server = (Server_t*)arg;
    struct sockaddr client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    /* Initialise epoll that will monitor the client file descriptor (required, since read is handled in separate function) */
    struct epoll_event ev, events[2]; // Max two events are expected at a time (socket fd and shutdown event fd)
    int epoll_fd = epoll_create1(0); // Create a new epoll instance and return a file descriptor referring to that instance
    if(epoll_fd == -1) {
        log_error("epoll_create1 returned -1 (err: %s)", strerror(errno));
        pthread_exit(NULL);
    }

    // Add the server listening socket file descriptor to the epoll event loop
    ev.events = EPOLLIN;
    ev.data.fd = server->fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server->fd, &ev) == -1) {
        log_error("epoll_ctl returned -1 (err: %s)", strerror(errno));
        pthread_exit(NULL);
    }

    // Add the disconnect event file descriptor to the epoll event loop
    ev.events = EPOLLIN;
    ev.data.fd = server->shutdown_eventfd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server->shutdown_eventfd, &ev) == -1) {
        log_error("epoll_ctl returned -1 (err: %s)", strerror(errno));
        pthread_exit(NULL);
    }

    // Run an infinite loop for monitoring server listening socket
    while(1) {
        int num_events = epoll_wait(epoll_fd, events, 2, -1);
        if(num_events == -1) {
            log_error("epoll_wait() failed (err: %s)", strerror(errno));
            break;
        }

        for(int i = 0; i < num_events; i++) {
            if(events[i].data.fd == server->fd) {
                // Handle incomming client request
                if(server->clients_list->get_length(server->clients_list) < server->cfg.max_clients) {
                    int client_fd = accept(server->fd, &client_addr, &client_addr_len); // blocks until a new client is present
                    if(client_fd == -1) {
                        log_error("accept() returned: -1 (err: %s)", strerror(errno));
                        // TODO: Clean the thread and client
                        pthread_exit(NULL);
                    }

                    // Get client IPv4 addr as a string
                    char client_addr_str[INET_ADDRSTRLEN];
                    struct sockaddr_in* client_addr_in = (struct sockaddr_in*)&client_addr;
                    struct in_addr addr = client_addr_in->sin_addr;
                    inet_ntop(AF_INET, &addr, client_addr_str, INET_ADDRSTRLEN);
                    log_info("new client accepted (ip: %s, fd: %d)", client_addr_str, client_fd);

                    // Create a new client handle
                    ServerClient_t new_client = { .fd = client_fd };

                    // Create an eventfd for synchronization
                    new_client.disconnect_eventfd = eventfd(0, EFD_NONBLOCK);
                    if(new_client.disconnect_eventfd == -1) {
                        log_error("eventfd() failed (err: %s)", strerror(errno));
                        pthread_exit(NULL);
                    }

                    // Create a new working thread for the client
                    ClientHandlerArgs_t args = { .client = new_client, .server = server };
                    int ret = pthread_create(&new_client.thread, NULL, server_client_handler, (void*)&args);
                    if(ret != 0) {
                        log_error("pthread_create() returned: %d", ret);
                        pthread_exit(NULL);
                    }
                    ret = pthread_detach(new_client.thread);
                    if(ret != 0) {
                        log_error("pthread_detach() returned: %d", ret);
                        pthread_exit(NULL);
                    }

                    // Add a new client to clients_list
                    ListError_t err =
                    server->clients_list->push(server->clients_list, &new_client, sizeof(ServerClient_t));
                    if(err != LIST_ERR_OK) {
                        log_error(
                        "failed to add a new client fd to the clients_fd_list, llist_push returned %d", err);
                        pthread_exit(NULL);
                    }

                    // Call a "client connect" handler
                    server->cfg.cb_list.on_client_connect(NULL, server, new_client);
                }
            } else if(events[i].data.fd == server->shutdown_eventfd) {
                // Handle shutdown request
                int err = close(server->fd); // Close server socket
                if(err == -1) {
                    log_error("close() returned: -1 (err: %s)", strerror(errno));
                }
                err = close(server->shutdown_eventfd); // Close eventfd
                if(err == -1) {
                    log_error("close() returned: -1 (err: %s)", strerror(errno));
                }
                err = close(epoll_fd); // Close epoll file descriptor
                if(err == -1) {
                    log_error("close() returned: -1 (err: %s)", strerror(errno));
                }
                log_info("exiting server thread");
                pthread_exit(NULL);
            }
        }
    }
    pthread_exit(NULL);
}

STATIC void* server_client_handler(void* arg) {
    if(!arg) {
        log_error("NULL ptr provided to server_listen - exiting the thread!");
        pthread_exit(NULL);
    }

    ServerClient_t client = ((ClientHandlerArgs_t*)arg)->client;
    Server_t* server = ((ClientHandlerArgs_t*)arg)->server;

    /* Initialise epoll that will monitor the client file descriptor (required, since read is handled in separate function) */
    struct epoll_event ev, events[2]; // Max two events are expected at a time (socket fd and disconnect event fd)
    int epoll_fd = epoll_create1(0); // Create a new epoll instance and return a file descriptor referring to that instance
    if(epoll_fd == -1) {
        log_error("epoll_create1 returned -1 (err: %s)", strerror(errno));
        pthread_exit(NULL);
    }

    // Add the client socket file descriptor to the epoll event loop
    ev.events = EPOLLIN;
    ev.data.fd = client.fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client.fd, &ev) == -1) {
        log_error("epoll_ctl returned -1 (err: %s)", strerror(errno));
        pthread_exit(NULL);
    }

    // Add the disconnect event file descriptor to the epoll event loop
    ev.events = EPOLLIN;
    ev.data.fd = client.disconnect_eventfd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client.disconnect_eventfd, &ev) == -1) {
        log_error("epoll_ctl returned -1 (err: %s)", strerror(errno));
        pthread_exit(NULL);
    }

    // Run an infinite loop for monitoring client socket
    while(1) {
        int num_events = epoll_wait(epoll_fd, events, 2, -1);
        if(num_events == -1) {
            log_error("epoll_wait() failed (err: %s)", strerror(errno));
            break;
        }

        for(int i = 0; i < num_events; i++) {
            if(events[i].data.fd == client.fd) {
                // Handle client data
                server->cfg.cb_list.on_data_received(NULL, server, client);
            } else if(events[i].data.fd == client.disconnect_eventfd) {
                // Disconnect signal received
                int err = close(client.fd); // Close client socket
                if(close(client.fd) == -1) {
                    log_error("close() returned: -1 (err: %s)", strerror(errno));
                }
                err = close(client.disconnect_eventfd); // Close eventfd
                if(err == -1) {
                    log_error("close() returned: -1 (err: %s)", strerror(errno));
                }
                err = close(epoll_fd); // Close epoll file descriptor
                if(err == -1) {
                    log_error("close() returned: -1 (err: %s)", strerror(errno));
                }
                log_info("exiting client thred (ID: %d, Socket fd: %d)", (int)client.thread, client.fd);
                pthread_exit(NULL);
            }
        }
    }
    pthread_exit(NULL);
}

STATIC int compare_client_fd(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

STATIC int server_create_socket(const char* port) {
    if(!port) {
        log_error("NULL ptr provided to server_create_socket");
        return -1;
    }
    /* Get the host address information (IP addr, port, etc.) */
    struct addrinfo hints, *server_addr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = SERVER_IP_VER;
    hints.ai_socktype = SERVER_SOCKTYPE;
    hints.ai_flags = AI_PASSIVE; // Socket addr is intended for bind()
    int ret = getaddrinfo(0, port, &hints, &server_addr);
    if(ret != 0) {
        log_error("getaddrinfo() returned: 0 (err: %s)", gai_strerror(ret));
        return -1;
    }

    /* Create an enpoint for communication and get the file descriptor that refers to that endpoint */
    int fd = socket(server_addr->ai_family, server_addr->ai_socktype, 0);
    if(fd == -1) {
        log_error("socket() returned: -1 (err: %s)", strerror(errno));
        freeaddrinfo(server_addr);
        return -1;
    }
    int option = 1; // Make the socket reuse recently orphaned addresses (fix for: address already in use)
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if(fd == -1) {
        log_error("setsockopt() returned: -1 (err: %s)", strerror(errno));
        freeaddrinfo(server_addr);
        return -1;
    }

    log_info("server socket created");

    /* Assign the address specified by ai_addr to the socket refered to by fd */
    ret = bind(fd, server_addr->ai_addr, server_addr->ai_addrlen);
    freeaddrinfo(server_addr);
    if(ret == -1) {
        log_error("bind() returned: -1 (err: %s)", strerror(errno));
        return -1;
    }
    log_info("assigned the IP addr to the server socket");

    return fd;
}

Server_t* server_create(const ServerConfig_t* cfg) {
    if(!cfg || !cfg->port || !cfg->cb_list.on_client_connect || !cfg->cb_list.on_client_disconnect ||
    !cfg->cb_list.on_data_received || !cfg->cb_list.on_server_shutdown) {
        return NULL;
    }

    int fd = server_create_socket(cfg->port);
    if(fd == -1) {
        log_error("server_create_socket() returned -1");
        return NULL;
    }

    Server_t* server = (Server_t*)malloc(sizeof(Server_t));
    if(!server) {
        log_error("malloc() returned NULL on new Server_t instance creation");
        return NULL;
    }
    /* Populate data in the struct (cfg, fd) and assign function pointers */
    memmove(&server->cfg, cfg, sizeof(ServerConfig_t));
    server->fd = fd;
    server->clients_list = llist_create(compare_client_fd); // TODO: this can return NULL, check before assigning
    if(server->clients_list == NULL) {
        log_error("llist_create() returned NULL when creating new clients list");
        free(server);
        server = NULL;
        return NULL;
    }
    server->run = server_run;
    server->read = server_read;
    server->write = server_write;
    server->broadcast = server_broadcast;
    server->disconnect = server_disconnect;
    server->shutdown = server_shutdown;
    server->destroy = server_destroy;

    return server;
}
