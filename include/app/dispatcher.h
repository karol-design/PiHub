/**
 * @file dispatcher.h
 * @brief A simple dispatcher for a PiHub commands that tokenizes the string, performs initial validation and
 * invokes adequate callback
 *
 * @note As per project requirements all commands should include 'target', 'action' and additional parameters.
 * @note Designed to provide thread-safe functionality (MT-Safe)
 *
 * @note Use dispatcher_init(), and dispatcher_deinit() to initialize and deinitialize new Dispatcher
 * instance. Use: dispatcher_register() and dispatcher_deregister() to add and remove new cmds definitions
 * (user is required to track CMD IDs). And finally use: dispatcher_execute() to parse the character string
 * buffer and execute the callback associated with the command (if no errors are encountered during parsing).
 */

#ifndef __CMD_DISPATCHER_H__
#define __CMD_DISPATCHER_H__

#include <pthread.h> // For: pthread_mutex_t and related function
#include <stdbool.h> // For: bool
#include <stdint.h>  // For: std types

#define DISPATCHER_MAX_CMD_COUNT 16   // Max number of commands that the dispatcher can handle
#define DISPATCHER_TARGET_MAX_SIZE 32 // Max size of the target character string token
#define DISPATCHER_ACTION_MAX_SIZE 32 // Max size of the action character string token
#define DISPATCHER_ARG_MAX_SIZE 32    // Max size of a single argument character string token
#define DISPATCHER_MAX_DELIM_SIZE 8   // Max size of the delimiter string in DispatcherConfig_t
#define DISPATCHER_MAX_ARGS 10        // Max number of arguments in the command

// Max size of the input buffer (one byte deliminer assumed)
#define DISPATCHER_MAX_BUF_SIZE \
    (DISPATCHER_TARGET_MAX_SIZE + 1 + DISPATCHER_ACTION_MAX_SIZE + 1 + (DISPATCHER_ARG_MAX_SIZE + 1) * DISPATCHER_MAX_ARGS)

typedef enum {
    DISPATCHER_ERR_OK = 0x00,        /**< Operation finished successfully */
    DISPATCHER_ERR_NULL_ARG,         /**< Error: NULL pointer passed as argument */
    DISPATCHER_ERR_INVALID_ARG,      /**< Error: Incorrect parameter passed */
    DISPATCHER_ERR_ID_ALREADY_TAKEN, /**< Error: Specified ID for the new command is already taken */
    DISPATCHER_ERR_CMD_NOT_FOUND,    /**< Error: Dispatcher could not identify the command */
    DISPATCHER_ERR_BUF_EMPTY,       /**< Error: Input buffer is empty or contains only deliminer characters */
    DISPATCHER_ERR_DELIM_TOO_LONG,  /**< Error: Delimiter too long */
    DISPATCHER_ERR_TOKEN_TOO_LONG,  /**< Error: One of the tokens exceeded the maximum allowed size*/
    DISPATCHER_ERR_BUF_TOO_LONG,    /**< Error: Input buffer is too long */
    DISPATCHER_ERR_CMD_INCOMPLETE,  /**< Error: The input buffer lacks action or other required token */
    DISPATCHER_ERR_TOO_MANY_ARGS,   /**< Error: To many arguments in the parsed cmd */
    DISPATCHER_ERR_PTHREAD_FAILURE, /**< Error: Pthread API call failure */
    DISPATCHER_ERR_GENERIC          /**< Error: Generic error */
} DispatcherError_t;

typedef struct {
    char delim[DISPATCHER_MAX_DELIM_SIZE]; // Delimiter string that should separate tokens in the command
} DispatcherConfig_t;

typedef struct {
    char target[DISPATCHER_TARGET_MAX_SIZE]; // Target token, e.g. "gpio", "sensor", "server"
    char action[DISPATCHER_ACTION_MAX_SIZE]; // Action token, e.g. "set", "get", "status"
    void (*callback_ptr)(char* argv, uint32_t argc, const void* cmd_ctx); // Pointer to the command handler
} DispatcherCommandDef_t;

typedef struct {
    bool valid;                 // To be always checked first
    DispatcherCommandDef_t cfg; // Command definition
} DispatcherCommand_t;

typedef struct Dispatcher {
    DispatcherConfig_t cfg;                                 // Dispatcher config
    DispatcherCommand_t cmd_list[DISPATCHER_MAX_CMD_COUNT]; // List of defined commands
    pthread_mutex_t lock;                                   // Lock for dispatcher-related critical sections
} Dispatcher_t;

/**
 * @brief Initialize a new Dispatcher instance
 * @param[in, out]  ctx  Pointer to the Dispatcher instance
 * @param[in] cfg Configuration structure
 * @return DISPATCHER_ERR_OK on success, DISPATCHER_ERR_NULL_ARG or DISPATCHER_ERR_PTHREAD_FAILURE otherwise
 */
DispatcherError_t dispatcher_init(Dispatcher_t* ctx, const DispatcherConfig_t cfg);

/**
 * @brief Add a new command definition to the Dispatcher
 * @param[in, out]  ctx  Pointer to the Dispatcher instance
 * @param[in]  id  ID of the new command
 * @param[in]  cmd  Command definition (incl. target, action and callback)
 * @return DISPATCHER_ERR_OK on success, DISPATCHER_ERR_NULL_ARG / DISPATCHER_ERR_INVALID_ARG /
 * DISPATCHER_ERR_PTHREAD_FAILURE / DISPATCHER_ERR_ID_ALREADY_TAKEN otherwise
 */
DispatcherError_t dispatcher_register(Dispatcher_t* ctx, const uint32_t id, const DispatcherCommandDef_t cmd);

/**
 * @brief Remove a command definition from the Dispatcher
 * @param[in, out]  ctx  Pointer to the Dispatcher instance
 * @param[in]  id  ID of the command to be removed
 * @return DISPATCHER_ERR_OK on success, DISPATCHER_ERR_NULL_ARG / DISPATCHER_ERR_INVALID_ARG /
 * DISPATCHER_ERR_PTHREAD_FAILURE otherwise
 * @note If the selected cmd is already invalid, this function has no effect
 */
DispatcherError_t dispatcher_deregister(Dispatcher_t* ctx, const uint32_t id);

/**
 * @brief Tokenize, validate and parse a command, then call the associated callback
 * @param[in, out]  ctx  Pointer to the Dispatcher instance
 * @param[in]  buf  NULL terminated character string with the command to be parsed
 * @param[in] cmd_ctx Pointer to the command execution context (e.g. details of the server's client that invoked this command)
 * @return DISPATCHER_ERR_OK on success, DISPATCHER_ERR_NULL_ARG / DISPATCHER_ERR_BUF_TOO_LONG / DISPATCHER_ERR_BUF_EMPTY /
 * DISPATCHER_ERR_CMD_INCOMPLETE / DISPATCHER_ERR_TOKEN_TOO_LONG / DISPATCHER_ERR_PTHREAD_FAILURE otherwise
 * @note Dispatcher will associate the parsed buf with the first cmd from the list that matches target and action
 */
DispatcherError_t dispatcher_execute(Dispatcher_t* ctx, const char* buf, const void* cmd_ctx);

/**
 * @brief Deinit the dispatcher (destroy the mutex)
 * @param[in, out]  ctx  Pointer to the Dispatcher instance
 * @return DISPATCHER_ERR_OK on success, DISPATCHER_ERR_NULL_ARG or DISPATCHER_ERR_PTHREAD_FAILURE otherwise
 */
DispatcherError_t dispatcher_deinit(Dispatcher_t* ctx);

#endif // __CMD_DISPATCHER_H__