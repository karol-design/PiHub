/**
 * @file server.h
 * @brief Manage a simple TCP/UDP server.
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

typedef enum {
    SERVER_ERR_OK = 0x00,           /**< Operation finished successfully */
    SERVER_ERR_NET_FAILURE,         /**< Error: System network API failure */
    SERVER_ERR_NULL_ARGUMENT,       /**< Error: NULL ptr passed as argument */
    SERVER_ERR_MALLOC_FAILED,       /**< Error: Dynamic memory allocation failed */
    SERVER_ERR_INCORRECT_PARAM,     /**< Error: Incorrect input parameter */
    SERVER_ERR_ACCEPT_FAILURE,      /**< Error: Accepting a new client failed */
    SERVER_ERR_PTHREAD_FAILURE,     /**< Error: Pthread API call failure */
    SERVER_ERR_CLIENT_DISCONNECTED, /**< Error: Client abruptly disconnected */
    SERVER_ERR_GENERIC,             /**< Error: Generic error */
} ServerError_t;

typedef struct {
    int fd;                 // Client's socket file descriptor
    int disconnect_eventfd; // Client's disconnect event file descriptor
    pthread_t thread;       // Client's worker thread ID
} ServerClient_t;

typedef struct {
    void (*on_client_connect)(void* ctx, void* this, ServerClient_t client);
    void (*on_data_received)(void* ctx, void* this, ServerClient_t client);
    void (*on_client_disconnect)(void* ctx, void* this, ServerClient_t client);
    void (*on_server_shutdown)(void* ctx, void* this);
} ServerCallbackList_t;

typedef struct {
    const char* port;                   // Port number as a string, e.g. "65001"
    const ServerCallbackList_t cb_list; // List of callbacks for network-related events
    const uint16_t max_clients;         // Maximum number of connected clients
    const uint16_t max_conn_requestes;  // Maximum number of waiting connection requests
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
    pthread_t listening_thread; // Server's listening thread ID
    ServerConfig_t cfg;         // Server config including port & callbacks
    List_t* clients_list;       // Pointer to a linked list with handles for active clients
    int shutdown_eventfd;       // Server's shutdown event file descriptor

    /**
     * @brief Start accepting new clients and create a listening thread
     * @param[in]  this  Pointer to the Server instance
     * @return SERVER_ERR_OK on success, SERVER_ERR_NULL_ARGUMENT / SERVER_ERR_NET_FAILURE / SERVER_ERR_PTHREAD_FAILURE otherwise
     */
    ServerError_t (*run)(struct Server* this);

    /**
     * @brief Read data from a client
     * @param[in]  this  Pointer to the Server instance
     * @param[in]  client  Handle of the client to which data should be sent
     * @param[out]  buf  Pointer to the memory where data will be stored
     * @param[in]  buf_length  Size of the buffer for the data
     * @param[out]  len  Pointer to a variable where length (in bytes) will be stored
     * @return SERVER_ERR_OK on success, SERVER_ERR_NULL_ARGUMENT or SERVER_ERR_CLIENT_DISCONNECTED otherwise
     * @note This function terminates the calling thread if the client disconnected
     */
    ServerError_t (*read)(struct Server* this, ServerClient_t client, uint8_t* buf, size_t buf_length, ssize_t* len);

    /**
     * @brief Send data to the client
     * @param[in]  this  Pointer to the Server instance
     * @param[in]  client  Handle of the client to which data should be sent
     * @param[in]  data  Pointer to the data to be sent
     * @param[in]  len  Length (in bytes) of the data
     * @return SERVER_ERR_OK on success, SERVER_ERR_NET_FAILURE otherwise
     */
    ServerError_t (*write)(struct Server* this, ServerClient_t client, const uint8_t* data, size_t len);

    /**
     * @brief Send data to all connected clients
     * @param[in]  this  Pointer to the Server instance
     * @param[in]  data  Pointer to the data to be sent
     * @param[in]  len  Length (in bytes) of the data
     * @return SERVER_ERR_OK on success, SERVER_ERR_NULL_ARGUMENT otherwise
     */
    ServerError_t (*broadcast)(struct Server* this, const uint8_t* data, size_t len);

    /**
     * @brief Get the char string with client's IPv4 address
     * @param[in]  this  Pointer to the Server instance
     * @param[in]  client  Handle of the client for which the address should be retrieved
     * @param[out]  inet_addrstr_buf  Pointer to the memory where data will be stored
     * @return SERVER_ERR_OK on success, SERVER_ERR_NULL_ARGUMENT or SERVER_ERR_NET_FAILURE otherwise
     * @note The inet_addrstr_buf buffer should be at least IPV4_ADDRSTR_LENGHT long
     */
    ServerError_t (*get_client_ip)(struct Server* this, ServerClient_t client, char* inet_addrstr_buf);

    /**
     * @brief Disconnect a client
     * @param[in]  this  Pointer to the Server instance
     * @param[in]  client  Handle of the client to be disconnected
     * @return SERVER_ERR_OK on success, SERVER_ERR_NET_FAILURE otherwise
     */
    ServerError_t (*disconnect)(struct Server* this, ServerClient_t client);

    /**
     * @brief Disconnect all clients and request the listening thread to exit via eventfd
     * @param[in]  this  Address of the pointer to the Server instance
     * @return SERVER_ERR_OK on success, SERVER_ERR_NULL_ARGUMENT or SERVER_ERR_GENERIC otherwise
     */
    ServerError_t (*shutdown)(struct Server* this);

    /**
     * @brief Destroy this instance of Server_t and free linked list with client fds
     * @param[in]  this  Address of the pointer to the Server instance
     * @return SERVER_ERR_OK on success, SERVER_ERR_NULL_ARGUMENT or SERVER_ERR_GENERIC otherwise
     * @note This MUST NOT be called on running server - always call shutdown() first.
     */
    ServerError_t (*destroy)(struct Server** this);
} Server_t;

/**
 * @brief Create a new server instance
 * @param[in]  cfg  Ptr to the configuration structure
 * @return SERVER_ERR_OK if succesful, SERVER_ERR_INIT_FAILURE otherwise
 */
Server_t* server_create(const ServerConfig_t* cfg);

#endif // __TCP_SERVER_H__