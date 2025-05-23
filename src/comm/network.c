#include "comm/network.h"

#include <errno.h>       // For: errno
#include <netdb.h>       // For: struct addrinfo, getaddrinfo(), socket()
#include <stdbool.h>     // For: bool
#include <string.h>      // For: memset, strerror
#include <sys/epoll.h>   // For: epoll*
#include <sys/eventfd.h> // Required for eventfd
#include <unistd.h>      // For: close

#include "utils/common.h"
#include "utils/log.h"

#define SERVER_IP_VER AF_INET        // IP version: IPv4
#define SERVER_SOCKTYPE SOCK_STREAM  // UDP/TCP: TCP
#define EPOLL_SERVER_LISTEN_EVENTS 2 // Two possible events: incoming client and shutdown request
#define EPOLL_CLIENT_THREAD_EVENTS 2 // Two possible events: incoming data or disconnect request

typedef struct {
    Server_t* server;
    ServerClient_t client;
} ClientHandlerArgs_t;

/**
 * @brief Create and bind a network socket
 * @param[in]  port  Port for which a socket should be created (e.g. "65001")
 * @param[out] fd Pointer to the int variable where the socket file descriptor will be saved
 * @return SERVER_ERR_OK on success, SERVER_ERR_NULL_ARGUMENT or SERVER_ERR_NET_FAILURE otherwise
 */
STATIC ServerError_t server_create_socket(const char* port, int* fd);

/**
 * @brief Run an infinite loop that will monitor server's incoming connections [Thread]
 * @param[in]  arg  Pointer to the Server instance
 * @return This function should never return
 */
STATIC void* server_listen(void* arg);

/**
 * @brief Run an infinite loop that waits for user input and calls appropriate callbacks [Thread]
 * @param[in]  arg  Pointer to the ClientHandlerArgs_t structure with ptrs to server and client objects
 * @return This function should not return
 */
STATIC void* server_client_handler(void* arg);

/**
 * @brief Handle a new client connection request
 * @param[in]  ctx  Pointer to the Server instance
 * @return SERVER_ERR_OK on success, SERVER_ERR_NET_FAILURE / SERVER_ERR_EVENTFD_FAILURE /
 * SERVER_ERR_PTHREAD_FAILURE / SERVER_ERR_LIST_FAILURE otherwise
 * @note This function blocks until a new connection request is present
 */
STATIC ServerError_t server_handle_conn_request(Server_t* ctx);

/**
 * @brief Callback for comparing two client file descriptors (required for Linked List)
 * @param[in]  a Pointer to the first ll data item
 * @param[in]  b Pointer to the second ll data item
 * @return Difference between a and b or 0 if they are equal
 */
STATIC int compare_client_fd(const void* a, const void* b);


ServerError_t server_init(Server_t* ctx, const ServerConfig_t cfg) {
    if(!ctx || !cfg.port || !cfg.cb_list.on_client_connect || !cfg.cb_list.on_client_disconnect ||
       !cfg.cb_list.on_data_received || !cfg.cb_list.on_server_failure) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    // Zero-out the Server_t struct and its members on init
    memset(ctx, 0, sizeof(Server_t));

    // Create a new listening socket for the server
    int fd;
    ServerError_t err = server_create_socket(cfg.port, &fd);
    if(err != SERVER_ERR_OK) {
        log_error("server_create_socket() returned %d", err);
        return SERVER_ERR_NET_FAILURE;
    }

    // Initialize mutex for protecting server-related critical sections
    int p_err = pthread_mutex_init(&ctx->lock, NULL);
    if(p_err != 0) {
        log_error("pthread_mutex_init() returned %d", p_err);
        return SERVER_ERR_PTHREAD_FAILURE;
    }

    // Populate data in the struct (cfg, fd) and create a llist for clients
    ctx->cfg = cfg;
    ctx->fd = fd;
    ListError_t ret = llist_init(&ctx->clients_list, compare_client_fd);
    if(ret != LIST_ERR_OK) {
        log_error("llist_init() returned %d when initializing new clients list", ret);
        return ret;
    }

    return SERVER_ERR_OK;
}

ServerError_t server_run(Server_t* ctx) {
    if(!ctx) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    // Start accepting connection on the socket
    int ret = listen(ctx->fd, ctx->cfg.max_conn_requests);
    if(ret == -1) {
        log_error("listen() returned: -1 (err: %s)", strerror(errno));
        return SERVER_ERR_NET_FAILURE;
    }
    log_info("server listening on port %s", ctx->cfg.port);

    // Create an eventfd for synchronization (shutdown request)
    ctx->shutdown_eventfd = eventfd(0, EFD_NONBLOCK);
    if(ctx->shutdown_eventfd == -1) {
        log_error("eventfd() failed (err: %s)", strerror(errno));
        return SERVER_ERR_EVENTFD_FAILURE;
    }

    // Create a listening thread
    pthread_t server_listening_thread;
    ret = pthread_create(&server_listening_thread, NULL, server_listen, (void*)ctx);
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

ServerError_t server_read(Server_t* ctx, ServerClient_t client, uint8_t* buf, const size_t buf_len, ssize_t* len) {
    if(!ctx || !buf || !len) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    // Read data from the client (critical section)
    int ret = pthread_mutex_lock(&client.lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return SERVER_ERR_PTHREAD_FAILURE;
    }
    log_debug("client lock taken");


    (*len) = recv(client.fd, (void*)buf, buf_len, 0);
    if(*len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        log_error("no data available to be read yet");
        return SERVER_ERR_OK; // No error, just wait for more data
    }

    if(*len <= 0) {
        return SERVER_ERR_CLIENT_DISCONNECTED;
    }

    ret = pthread_mutex_unlock(&client.lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return SERVER_ERR_PTHREAD_FAILURE;
    }
    log_debug("client lock released");


    log_debug("received %lu bytes from the client (fd: %d)", *len, client.fd);
    return SERVER_ERR_OK;
}

ServerError_t server_write(const Server_t* ctx, ServerClient_t client, const uint8_t* data, const size_t len) {
    if(!ctx || !data) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    // Write data to the client (critical section)
    int ret = pthread_mutex_lock(&client.lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return SERVER_ERR_PTHREAD_FAILURE;
    }
    log_debug("client lock taken");


    ssize_t bytes_sent = 0;
    while(bytes_sent < len) {
        ssize_t bytes = send(client.fd, data + bytes_sent, len - bytes_sent, 0);
        if(bytes <= 0) {
            return SERVER_ERR_NET_FAILURE;
        }
        bytes_sent += bytes;
    }

    ret = pthread_mutex_unlock(&client.lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return SERVER_ERR_PTHREAD_FAILURE;
    }
    log_debug("client lock released");


    log_debug("%lu bytes sent to the client (fd: %d)", bytes_sent, client.fd);
    return SERVER_ERR_OK;
}

ServerError_t server_broadcast(Server_t* ctx, const uint8_t* data, const size_t len) {
    if(!ctx || !data) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    ListNode_t* client = ctx->clients_list.get_head(&ctx->clients_list);
    ServerClient_t client_fd;

    // Write data to all clients (if any)
    while(client) {
        client_fd = *(ServerClient_t*)(client->data);
        ServerError_t err = server_write(ctx, client_fd, data, len);
        if(err != SERVER_ERR_OK) {
            return err;
        }
        client = client->next; // @TODO: There should be a dedicated ll function for this protected with mutexes (risk of accessing deleted node)
    }

    return SERVER_ERR_OK;
}

ServerError_t server_disconnect(Server_t* ctx, const ServerClient_t client) {
    if(!ctx) {
        return SERVER_ERR_NULL_ARGUMENT;
    }
    // @TODO: Add a lock to protect the disconnect procedure from race conditions

    uint64_t signal_value = 1;
    ssize_t ret = write(client.disconnect_eventfd, &signal_value, sizeof(signal_value));
    if(ret != sizeof(signal_value)) {
        log_error("failed to write to disconnect_eventfd (err: %s)", strerror(errno));
        return SERVER_ERR_EVENTFD_FAILURE;
    }

    return SERVER_ERR_OK;
}

ServerError_t server_shutdown(Server_t* ctx) {
    if(!ctx) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    // Shutdown the server (critical section)
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return SERVER_ERR_PTHREAD_FAILURE;
    }
    log_debug("server lock taken");

    ListNode_t* node = ctx->clients_list.get_head(&ctx->clients_list);
    ServerError_t err = SERVER_ERR_OK;
    ServerClient_t* client;

    // Disconnect all clients first
    while(node) {
        client = (ServerClient_t*)node->data;
        node = node->next;
        err = server_disconnect(ctx, *client);
        if(err != SERVER_ERR_OK) {
            return err;
        }
    }

    // Write to shutdown eventfd to stop server's listening thread
    uint64_t signal_value = 1;
    ssize_t bytes = write(ctx->shutdown_eventfd, &signal_value, sizeof(signal_value));
    if(bytes != sizeof(signal_value)) {
        log_error("failed to write to shutdown_eventfd (err: %s)", strerror(errno));
        return SERVER_ERR_EVENTFD_FAILURE;
    }

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return SERVER_ERR_PTHREAD_FAILURE;
    }
    log_debug("server lock released");

    return SERVER_ERR_OK;
}

ServerError_t server_deinit(Server_t* ctx) {
    if(!ctx) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    // @TODO: Add a check if server is not running

    // Destroy the server (critical section)
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return SERVER_ERR_PTHREAD_FAILURE;
    }
    log_debug("server lock taken");

    ListError_t err = ctx->clients_list.deinit(&(ctx->clients_list));
    if(err != LIST_ERR_OK) {
        log_error("failed to destroy linked list with client FDs (llist_destroy() returned: %d)", err);
        return SERVER_ERR_LLIST_FAILURE;
    }

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return SERVER_ERR_PTHREAD_FAILURE;
    }
    log_debug("server lock released");

    // Destroy the lock (@TODO: Improve mutex destroy mechanism, e.g. add a global protection variable)
    ret = pthread_mutex_destroy(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_destroy() returned %d", ret);
        return SERVER_ERR_PTHREAD_FAILURE;
    }

    // Zero-out the Server_t struct along with its members on deinit
    memset(ctx, 0, sizeof(Server_t));

    return SERVER_ERR_OK;
}

ServerError_t server_get_client_ip(const ServerClient_t client, char* inet_addrstr_) {
    if(!inet_addrstr_) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    // Retrieve the address associated with the client's socket file descriptor
    struct sockaddr addr;
    int addr_len = sizeof(struct sockaddr_in);
    int ret = getpeername(client.fd, &addr, &addr_len);
    if(ret == -1) {
        log_error("getpeername failed and returned -1 (err: %s)", strerror(errno));
        return SERVER_ERR_NET_FAILURE;
    }

    struct sockaddr_in* addr_in = (struct sockaddr_in*)&addr;
    struct in_addr ip_addr = addr_in->sin_addr;

    // Convert the address structure into a character string with dotted notation IPv4 addr
    char ip_addr_str[INET_ADDRSTRLEN];
    const char* inet_ret = inet_ntop(SERVER_IP_VER, &ip_addr, ip_addr_str, INET_ADDRSTRLEN);
    if(inet_ret == NULL) {
        log_error("failed to convert the ip addr struct to a char string (err: %s)", strerror(errno));
        return SERVER_ERR_NET_FAILURE;
    }

    // If the addr was retrieved and converted properly copy it to the output buffer
    memmove(inet_addrstr_, ip_addr_str, sizeof(ip_addr_str));

    return SERVER_ERR_OK;
}

ListNode_t* server_get_clients(Server_t* ctx) {
    if(!ctx) {
        return NULL;
    }

    return ctx->clients_list.get_head(&ctx->clients_list);
}

STATIC ServerError_t server_create_socket(const char* port, int* fd) {
    if(!port) {
        return SERVER_ERR_NULL_ARGUMENT;
    }

    /* Get the host address information (IP addr, port, etc.) */
    struct addrinfo hints;
    struct addrinfo* server_addr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = SERVER_IP_VER;
    hints.ai_socktype = SERVER_SOCKTYPE;
    hints.ai_flags = AI_PASSIVE; // Socket addr is intended for bind()
    int ret = getaddrinfo(0, port, &hints, &server_addr);
    if(ret != 0) {
        log_error("getaddrinfo() returned: 0 (err: %s)", gai_strerror(ret));
        return SERVER_ERR_NET_FAILURE;
    }

    /* Create an endpoint for communication and get the file descriptor that refers to that endpoint */
    *fd = socket(server_addr->ai_family, server_addr->ai_socktype, 0);
    if(*fd == -1) {
        log_error("socket() returned: -1 (err: %s)", strerror(errno));
        freeaddrinfo(server_addr);
        return SERVER_ERR_NET_FAILURE;
    }
    int option = 1; // Make the socket reuse recently orphaned addresses (fix for: address already in use)
    setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if(*fd == -1) {
        log_error("setsockopt() returned: -1 (err: %s)", strerror(errno));
        freeaddrinfo(server_addr);
        return SERVER_ERR_NET_FAILURE;
    }

    log_info("server socket created (port: %s, fd: %d)", port, *fd);

    /* Assign the address specified by ai_addr to the socket refered to by fd */
    ret = bind(*fd, server_addr->ai_addr, server_addr->ai_addrlen);
    freeaddrinfo(server_addr);
    if(ret == -1) {
        log_error("bind() returned: -1 (err: %s)", strerror(errno));
        return SERVER_ERR_NET_FAILURE;
    }
    log_info("assigned the IP addr to the server socket");

    return SERVER_ERR_OK;
}

STATIC void* server_listen(void* arg) {
    if(!arg) {
        log_error("NULL ptr provided to server_listen; exiting server listening thread");
        pthread_exit(NULL);
    }

    Server_t* server = (Server_t*)arg;

    // Initialise epoll that will monitor the client file descriptor (required, since read is handled in separate function)
    struct epoll_event ev, events[2]; // Max two events are expected at a time (socket fd and shutdown event fd)
    int epoll_fd = epoll_create1(0); // Create a new epoll instance and return a file descriptor referring to that instance
    if(epoll_fd == -1) {
        log_error("epoll_create1 returned -1 (err: %s); exiting server listening thread", strerror(errno));
        server->cfg.cb_list.on_server_failure(server, SERVER_ERR_EPOLL_FAILURE);
        pthread_exit(NULL);
    }

    // Add the server listening socket file descriptor to the epoll event loop
    ev.events = EPOLLIN;
    ev.data.fd = server->fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server->fd, &ev) == -1) {
        log_error("epoll_ctl returned -1 (err: %s); exiting server listening thread", strerror(errno));
        server->cfg.cb_list.on_server_failure(server, SERVER_ERR_EPOLL_FAILURE);
        pthread_exit(NULL);
    }

    // Add the disconnect event file descriptor to the epoll event loop
    ev.events = EPOLLIN;
    ev.data.fd = server->shutdown_eventfd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server->shutdown_eventfd, &ev) == -1) {
        log_error("epoll_ctl returned -1 (err: %s); exiting server listening thread", strerror(errno));
        server->cfg.cb_list.on_server_failure(server, SERVER_ERR_EPOLL_FAILURE);
        pthread_exit(NULL);
    }

    // Run an infinite loop for monitoring server listening socket
    while(1) {
        int num_events = epoll_wait(epoll_fd, events, EPOLL_SERVER_LISTEN_EVENTS, -1);
        if(num_events == -1) {
            log_error("epoll_wait() failed (err: %s); exiting server listening thread", strerror(errno));
            server->cfg.cb_list.on_server_failure(server, SERVER_ERR_EPOLL_FAILURE);
            pthread_exit(NULL);
        }

        // Handle all queued events
        for(int i = 0; i < num_events; i++) {
            if(events[i].data.fd == server->fd) {
                // Handle an incoming connection request
                ServerError_t err = server_handle_conn_request(server);
                if(err != SERVER_ERR_OK) {
                    log_error("server_handle_conn_request() returned %d; exiting server listening thread", err);
                    server->cfg.cb_list.on_server_failure(server, err);
                    pthread_exit(NULL);
                }
            } else if(events[i].data.fd == server->shutdown_eventfd) {
                // Handle shutdown request
                // @TODO: Handle (reject) all queued incoming connection requests and improve error hanling
                int err = close(server->fd); // Close server socket
                if(err != 0) {
                    log_error("close() returned: -1 (err: %s)", strerror(errno));
                }
                err += close(server->shutdown_eventfd); // Close eventfd
                if(err != 0) {
                    log_error("close() returned: -1 (err: %s)", strerror(errno));
                }
                err += close(epoll_fd); // Close epoll file descriptor
                if(err != 0) {
                    log_error("close() returned: -1 (err: %s)", strerror(errno));
                }
                if(err != 0) {
                    server->cfg.cb_list.on_server_failure(server, SERVER_ERR_GENERIC); // Failure when cleaning server's resources
                }
                log_info("exiting server listening thread (fd: %d)", server->fd);
                pthread_exit(NULL);
            }
        }
    }
    return NULL; // This function (thread) should never return
}

STATIC void* server_client_handler(void* arg) {
    if(!arg) {
        log_error("NULL ptr provided to server_listen; exiting client thread!");
        pthread_exit(NULL);
    }

    ServerClient_t client = ((ClientHandlerArgs_t*)arg)->client;
    Server_t* server = ((ClientHandlerArgs_t*)arg)->server;
    bool self_disconnect = false;

    // Initialise epoll that will monitor the client file descriptor (required, since read is handled in separate function)
    struct epoll_event ev, events[2]; // Max two events are expected at a time (socket fd and disconnect event fd)
    int epoll_fd = epoll_create1(0); // Create a new epoll instance and return a file descriptor referring to that instance
    if(epoll_fd == -1) {
        log_error("epoll_create1 returned -1 (err: %s)", strerror(errno));
        server->cfg.cb_list.on_server_failure(server, SERVER_ERR_EPOLL_FAILURE);
        pthread_exit(NULL);
    }

    // Add the client socket file descriptor to the epoll event loop
    ev.events = EPOLLIN;
    ev.data.fd = client.fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client.fd, &ev) == -1) {
        log_error("epoll_ctl returned -1 (err: %s)", strerror(errno));
        server->cfg.cb_list.on_server_failure(server, SERVER_ERR_EPOLL_FAILURE);
        pthread_exit(NULL);
    }

    // Add the disconnect event file descriptor to the epoll event loop
    ev.events = EPOLLIN;
    ev.data.fd = client.disconnect_eventfd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client.disconnect_eventfd, &ev) == -1) {
        log_error("epoll_ctl returned -1 (err: %s)", strerror(errno));
        server->cfg.cb_list.on_server_failure(server, SERVER_ERR_EPOLL_FAILURE);
        pthread_exit(NULL);
    }

    // Run an infinite loop for monitoring client socket
    while(1) {
        int num_events = epoll_wait(epoll_fd, events, EPOLL_CLIENT_THREAD_EVENTS, -1);
        if(num_events == -1) {
            log_error("epoll_wait() failed (err: %s)", strerror(errno));
            server->cfg.cb_list.on_server_failure(server, SERVER_ERR_EPOLL_FAILURE);
            pthread_exit(NULL);
        }

        for(int i = 0; i < num_events; i++) {
            if(events[i].data.fd == client.fd) {
                // Check if the client has sent new data or disconnected
                char tmp;
                ssize_t peek_ret = recv(client.fd, &tmp, 1, MSG_PEEK | MSG_DONTWAIT);
                if(peek_ret == 0) {
                    // Client has disconnected, trigger disconnect procedure
                    self_disconnect = true;
                } else if(peek_ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    // Error, treat as disconnect
                    self_disconnect = true;
                } else {
                    // Only call on_data_received if there is data
                    server->cfg.cb_list.on_data_received(server, client);
                }
            }

            if(events[i].data.fd == client.disconnect_eventfd || self_disconnect == true) {
                // Disconnect signal received
                log_info("client (fd: %d) to be disconnected from the server", client.fd);

                int err = close(client.fd); // Close client socket
                if(err != 0) {
                    log_error("close() returned: -1 (err: %s)", strerror(errno));
                }
                err += close(client.disconnect_eventfd); // Close eventfd
                if(err != 0) {
                    log_error("close() returned: -1 (err: %s)", strerror(errno));
                }
                err += close(epoll_fd); // Close epoll file descriptor
                if(err != 0) {
                    log_error("close() returned: -1 (err: %s)", strerror(errno));
                }
                err += (int)server->clients_list.remove(&server->clients_list, &client); // Remove client from the server's clients list
                if(err != LIST_ERR_OK) {
                    log_error("failed to remove the client from the clients_list (err: %d)", err);
                }
                if(err != 0) {
                    server->cfg.cb_list.on_server_failure(server, SERVER_ERR_GENERIC); // Failure when cleaning client's resources
                }
                err += pthread_mutex_destroy(&client.lock); // Destroy the lock (@TODO: improve mutex destroy algorithm)
                if(err != 0) {
                    log_error("pthread_mutex_destroy() returned %d", err);
                }

                if(self_disconnect) {
                    // Call "client disconnect" handler only on self disconnect (not on forced disconnect or during shutdown)
                    server->cfg.cb_list.on_client_disconnect(server, client);
                }

                log_info("exiting client thread (id: %lu, fd: %d)", client.thread, client.fd);
                pthread_exit(NULL);
            }
        }
    }
    return NULL; // This function (thread) should never return
}

STATIC int compare_client_fd(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

STATIC ServerError_t server_handle_conn_request(Server_t* ctx) {
    if(!ctx) {
        return SERVER_ERR_NULL_ARGUMENT;
    }
    struct sockaddr client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Block until a new client is present
    int client_fd = accept(ctx->fd, &client_addr, &client_addr_len);
    if(client_fd == -1) {
        log_error("accept() returned: -1 (err: %s)", strerror(errno));
        return SERVER_ERR_NET_FAILURE;
    }

    // Check if the new client request can be accepted and reject (close) if there is no more space for new clients
    if(ctx->clients_list.get_length(&ctx->clients_list) >= ctx->cfg.max_clients) {
        int err = close(client_fd);
        if(err == -1) {
            log_error("close() returned: -1 (err: %s)", strerror(errno));
            return SERVER_ERR_NET_FAILURE;
        }
        log_error("new connection request dropped (max no of clients [%d] reached)", ctx->cfg.max_clients);
        return SERVER_ERR_OK;
    }

    // Create a new client handle
    ServerClient_t client = { .fd = client_fd };

    // Initialize mutex for protecting client-related critical sections
    int ret = pthread_mutex_init(&client.lock, NULL);
    if(ret != 0) {
        log_error("pthread_mutex_init() returned %d", ret);
        return SERVER_ERR_PTHREAD_FAILURE;
    }

    // Create an eventfd for synchronization
    client.disconnect_eventfd = eventfd(0, EFD_NONBLOCK);
    if(client.disconnect_eventfd == -1) {
        log_error("eventfd() failed (err: %s)", strerror(errno));
        return SERVER_ERR_EVENTFD_FAILURE;
    }

    // Create a new working thread for the client
    ClientHandlerArgs_t args = { .client = client, .server = ctx };
    ret = pthread_create(&client.thread, NULL, server_client_handler, (void*)&args);
    if(ret != 0) {
        log_error("pthread_create() returned: %d", ret);
        return SERVER_ERR_PTHREAD_FAILURE;
    }
    ret = pthread_detach(client.thread);
    if(ret != 0) {
        log_error("pthread_detach() returned: %d", ret);
        return SERVER_ERR_PTHREAD_FAILURE;
    }

    // Add a new client to clients_list
    ListError_t err = ctx->clients_list.push(&ctx->clients_list, &client, sizeof(ServerClient_t));
    if(err != LIST_ERR_OK) {
        log_error("failed to add a new client fd to the clients_fd_list, llist_push returned %d", err);
        return SERVER_ERR_LLIST_FAILURE;
    }

    // Get client IPv4 addr as a string
    char ip_addr[IPV4_ADDRSTR_LENGTH];
    if(server_get_client_ip(client, ip_addr) == SERVER_ERR_OK) {
        log_info("new client accepted (ip: %s, fd: %d, thread: %lu)", ip_addr, client.fd, client.thread);
    }

    // Call a "client connect" handler
    ctx->cfg.cb_list.on_client_connect(ctx, client);
    return SERVER_ERR_OK;
}
