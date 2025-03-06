/**
 * @file server.h
 * @brief Manage a simple TCP/UDP server.
 *
 * @note Use server_create(), server->run(), server->shutdown() and server->destroy(). Use read(), write(), broadcast(),
 * disconnect(), get_clients() and get_client_ip() to mange active clients (incl. I/O) [this part of the API is thread-safe].
 *
 * Additional control is provided via callbacks for events such as: client_connect (called by: server listening thread),
 * data_received & client_disconnect (called by: client worker thread) and server_failure (called by: server listening thread
 * OR client worker thread).
 *
 * @note Multithreading: This component creates one thread for server control (e.g. shutdown requests) and for listening
 * to incomming data. A worker thread is also created for each new client. [no. of threads per instance = 1 + clients_count]
 */

#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include <arpa/inet.h> // For: inet_ntop
#include <pthread.h>   // For: pthread_mutex_t
#include <stdbool.h>   // For: bool
#include <stdint.h>    // For: std types
#include <stdlib.h>    // For: size_t

#include "utils/list.h" // For: List_t

#define IPV4_ADDRSTR_LENGHT INET_ADDRSTRLEN
#define MAX_PORTSTR_LENGTH 12

/**
 * @struct ServerError_t
 * @brief Error codes returned by server API functions
 */
typedef enum {
    SERVER_ERR_OK = 0x00,           /**< Operation finished successfully */
    SERVER_ERR_NET_FAILURE,         /**< Error: System network API failure */
    SERVER_ERR_NULL_ARGUMENT,       /**< Error: NULL ptr passed as argument */
    SERVER_ERR_MALLOC_FAILURE,      /**< Error: Dynamic memory allocation failed */
    SERVER_ERR_PTHREAD_FAILURE,     /**< Error: Pthread API call failure */
    SERVER_ERR_EVENTFD_FAILURE,     /**< Error: Eventfd API call failure */
    SERVER_ERR_LLIST_FAILURE,       /**< Error: Linked List API call failure */
    SERVER_ERR_EPOLL_FAILURE,       /**< Error: Epoll API call failure */
    SERVER_ERR_CLIENT_DISCONNECTED, /**< Error: Client abruptly disconnected */
    SERVER_ERR_GENERIC,             /**< Error: Generic error */
} ServerError_t;

/**
 * @struct ServerClient_t
 * @brief Client context structure with socket file descriptor, eventfd fds, and thread handle
 */
typedef struct {
    int fd;                 // Client's socket file descriptor
    int disconnect_eventfd; // Client's disconnect event file descriptor
    pthread_t thread;       // Client's worker thread ID
    pthread_mutex_t lock; // Client's lock for critical sections with ops on client's shared resources and I/O
} ServerClient_t;

/**
 * @struct ServerCallbackList_t
 * @brief List of callback pointers for key server events
 */
typedef struct {
    void (*on_client_connect)(void* ctx, const ServerClient_t client);
    void (*on_data_received)(void* ctx, const ServerClient_t client);
    void (*on_client_disconnect)(void* ctx, const ServerClient_t client);
    void (*on_server_failure)(void* ctx, const ServerError_t err);
} ServerCallbackList_t;

/**
 * @struct ServerConfig_t
 * @brief Include port number, list of callbacks, and max clients/requests number
 * @note This should be set/modified only once - before being passed to server_create() which will copy the content
 */
typedef struct {
    char port[MAX_PORTSTR_LENGTH]; // Port number as a string, e.g. "65001"
    ServerCallbackList_t cb_list;  // List of callbacks for network-related events
    uint16_t max_clients;          // Maximum number of connected clients
    uint16_t max_conn_requests;    // Maximum number of waiting connection requests
} ServerConfig_t;

/**
 * @struct Server_t
 * @brief Include configuration data, socket fd, ll with client fds, and pointers to member functions.
 * @note Since the Server object can be shared between multiple threads (e.g. client threads), all data
 * members (apart from the Linked List, which is thread-safe) are immutable and therefore have to be defined
 * during the new Server object (instance) creation.
 */
typedef struct Server {
    int32_t fd;                 // Listening socket file descriptor
    ServerConfig_t cfg;         // Server config including port & callbacks
    List_t clients_list;        // Linked list with handles for active clients
    pthread_mutex_t lock;       // Lock for server-related critical sections
    int shutdown_eventfd;       // Server's shutdown event file descriptor
    pthread_t listening_thread; // Server's listening thread ID

    /**
     * @brief Start accepting new clients and create a listening thread
     * @param[in]  ctx  Pointer to the Server instance
     * @return SERVER_ERR_OK on success, SERVER_ERR_NULL_ARGUMENT / SERVER_ERR_NET_FAILURE / SERVER_ERR_PTHREAD_FAILURE otherwise
     */
    ServerError_t (*run)(struct Server* ctx);

    /**
     * @brief Read data from a client
     * @param[in]  ctx  Pointer to the Server instance
     * @param[in]  client  Handle of the client to which data should be sent
     * @param[out]  buf  Pointer to the memory where data will be stored
     * @param[in]  size  Size of the buffer for the data
     * @param[out]  len  Pointer to a variable where length (in bytes) will be stored
     * @return SERVER_ERR_OK on success, SERVER_ERR_NULL_ARGUMENT or SERVER_ERR_CLIENT_DISCONNECTED otherwise
     * @note This function terminates the calling thread if the client disconnected
     */
    ServerError_t (*read)(struct Server* ctx, ServerClient_t client, uint8_t* buf, const size_t buf_len, ssize_t* len);

    /**
     * @brief Send data to the client
     * @param[in]  ctx  Pointer to the Server instance
     * @param[in]  client  Handle of the client to which data should be sent
     * @param[in]  data  Pointer to the data to be sent
     * @param[in]  len  Length (in bytes) of the data
     * @return SERVER_ERR_OK on success, SERVER_ERR_NET_FAILURE otherwise
     */
    ServerError_t (*write)(const struct Server* ctx, ServerClient_t client, const uint8_t* data, const size_t len);

    /**
     * @brief Send data to all connected clients
     * @param[in]  ctx  Pointer to the Server instance
     * @param[in]  data  Pointer to the data to be sent
     * @param[in]  len  Length (in bytes) of the data
     * @return SERVER_ERR_OK on success, SERVER_ERR_NULL_ARGUMENT otherwise
     */
    ServerError_t (*broadcast)(struct Server* ctx, const uint8_t* data, size_t len);

    /**
     * @brief Get the char string with client's IPv4 address
     * @param[in]  client  Handle of the client for which the address should be retrieved
     * @param[out]  inet_addrstr_buf  Pointer to the memory where data will be stored
     * @return SERVER_ERR_OK on success, SERVER_ERR_NULL_ARGUMENT or SERVER_ERR_NET_FAILURE otherwise
     * @note The inet_addrstr_buf buffer should be at least IPV4_ADDRSTR_LENGHT long
     */
    ServerError_t (*get_client_ip)(const ServerClient_t client, char* inet_addrstr_buf);

    /**
     * @brief Get the head of the linked list with all connected clients
     * @param[in]  ctx  Pointer to the Server instance
     * @return Head of LL with ServerClient_t structs on success, NULL if empty or on error
     */
    ListNode_t* (*get_clients)(struct Server* ctx);

    /**
     * @brief Disconnect a client
     * @param[in]  ctx  Pointer to the Server instance
     * @param[in]  client  Handle of the client to be disconnected
     * @param[in]  no_disconnect  Flag indicating whether on_disconnect callback should be invoked (can cause deadlock, e.g. when called by shutdown())
     * @return SERVER_ERR_OK on success, SERVER_ERR_NET_FAILURE otherwise
     */
    ServerError_t (*disconnect)(struct Server* ctx, const ServerClient_t client, bool no_callback);

    /**
     * @brief Disconnect all clients and request the listening thread to exit via eventfd
     * @param[in]  ctx  Address of the pointer to the Server instance
     * @return SERVER_ERR_OK on success, SERVER_ERR_NULL_ARGUMENT or SERVER_ERR_GENERIC otherwise
     */
    ServerError_t (*shutdown)(struct Server* ctx);

    /**
     * @brief Deinit the server
     * @param[in, out]  ctx  Pointer to the Server instance
     * @return SERVER_ERR_OK on success, SERVER_ERR_NULL_ARGUMENT or SERVER_ERR_GENERIC otherwise
     * @note This MUST NOT be called on running server - always call shutdown() first.
     */
    ServerError_t (*deinit)(struct Server* ctx);
} Server_t;

/**
 * @brief Initialize a new Parser instance
 * @param[in, out]  ctx  Pointer to the Server instance
 * @param[in]  cfg  Configuration structure
 * @return SERVER_ERR_OK on success, SERVER_ERR_NULL_ARG or SERVER_ERR_PTHREAD_FAILURE otherwise
 */
ServerError_t server_init(Server_t* ctx, const ServerConfig_t cfg);

#endif // __TCP_SERVER_H__
