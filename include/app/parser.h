/**
 * @file parser.h
 * @brief A simple parser for a PiHub commands that tokenizes the string, performs initial validation and invokes adequate callback
 *
 * @note As per project requirements all commands should include 'target', 'action' and additional parameters.
 * @note Designed to provide thread-safe functionality (MT-Safe)
 *
 * @note Use parser_init(), and parser.uninit() to initialize and deinitialize new Parser instance. Use: parser.add_cmd() and parser.remove_cmd()
 * to add and remove new cmds definitions (user is required to track CMD IDs). And finally use: parser.execute() to parse the character string
 * buffer and execute the callback associated with the command (if no errors are encountered during parsing).
 */

#ifndef __CMD_PARSER_H__
#define __CMD_PARSER_H__

#include <pthread.h> // For: pthread_mutex_t and related function
#include <stdbool.h> // For: bool
#include <stdint.h>  // For: std types

#define PARSER_MAX_CMD_COUNT 16   // Max number of commands that the parser can handle
#define PARSER_TARGET_MAX_SIZE 32 // Max size of the target character string token
#define PARSER_ACTION_MAX_SIZE 32 // Max size of the action character string token
#define PARSER_ARG_MAX_SIZE 32    // Max size of a single argument character string token
#define PARSER_MAX_DELIM_SIZE 8   // Max size of the delimiter string in ParserConfig_t
#define PARSER_MAX_ARGS 10        // Max number of arguments in the command

// Max size of the input buffer (one byte deliminer assumed)
#define PARSER_MAX_BUF_SIZE \
    (PARSER_TARGET_MAX_SIZE + 1 + PARSER_ACTION_MAX_SIZE + 1 + (PARSER_ARG_MAX_SIZE + 1) * PARSER_MAX_ARGS)

typedef enum {
    PARSER_ERR_OK = 0x00,        /**< Operation finished successfully */
    PARSER_ERR_NULL_ARG,         /**< Error: NULL pointer passed as argument */
    PARSER_ERR_INVALID_ARG,      /**< Error: Incorrect parameter passed */
    PARSER_ERR_ID_ALREADY_TAKEN, /**< Error: Specified ID for the new command is already taken */
    PARSER_ERR_CMD_NOT_FOUND,    /**< Error: Parser could not identify the command */
    PARSER_ERR_BUF_EMPTY,        /**< Error: Input buffer is empty or contains only deliminer characters */
    PARSER_ERR_DELIM_TOO_LONG,   /**< Error: Delimiter too long */
    PARSER_ERR_TOKEN_TOO_LONG,   /**< Error: One of the tokens exceeded the maximum allowed size*/
    PARSER_ERR_BUF_TOO_LONG,     /**< Error: Input buffer is too long */
    PARSER_ERR_CMD_INCOMPLETE,   /**< Error: The input buffer lacks action or other required token */
    PARSER_ERR_TOO_MANY_ARGS,    /**< Error: To many arguments in the parsed cmd */
    PARSER_ERR_PTHREAD_FAILURE,  /**< Error: Pthread API call failure */
    PARSER_ERR_GENERIC           /**< Error: Generic error */
} ParserError_t;

typedef struct {
    char delim[PARSER_MAX_DELIM_SIZE]; // Delimiter string that should separate tokens in the command
} ParserConfig_t;

typedef struct {
    char target[PARSER_TARGET_MAX_SIZE];             // Target token, e.g. "gpio", "sensor", "server"
    char action[PARSER_ACTION_MAX_SIZE];             // Action token, e.g. "set", "get", "status"
    void (*callback_ptr)(char* argv, uint32_t argc); // Pointer to the command handler
} ParserCommandDef_t;

typedef struct {
    bool valid;             // To be always checked first
    ParserCommandDef_t cfg; // Command definition
} ParserCommand_t;

typedef struct Parser {
    ParserConfig_t cfg;                             // Parser config
    ParserCommand_t cmd_list[PARSER_MAX_CMD_COUNT]; // List of defined commands
    pthread_mutex_t lock;                           // Lock for parser-related critical sections

    /**
     * @brief Add a new command definition to the Parser
     * @param[in, out]  ctx  Pointer to the Parser instance
     * @param[in]  id  ID of the new command
     * @param[in]  cmd  Command definition (incl. target, action and callback)
     * @return PARSER_ERR_OK on success, PARSER_ERR_NULL_ARG / PARSER_ERR_INVALID_ARG /
     * PARSER_ERR_PTHREAD_FAILURE / PARSER_ERR_ID_ALREADY_TAKEN otherwise
     */
    ParserError_t (*add_cmd)(struct Parser* ctx, const uint32_t id, const ParserCommandDef_t cmd);

    /**
     * @brief Remove a command definition from the Parser
     * @param[in, out]  ctx  Pointer to the Parser instance
     * @param[in]  id  ID of the command to be removed
     * @return PARSER_ERR_OK on success, PARSER_ERR_NULL_ARG / PARSER_ERR_INVALID_ARG /
     * PARSER_ERR_PTHREAD_FAILURE otherwise
     * @note If the selected cmd is already invalid, this function has no effect
     */
    ParserError_t (*remove_cmd)(struct Parser* ctx, const uint32_t id);

    /**
     * @brief Tokenize, validate and parse a command, then call the associated callback
     * @param[in, out]  ctx  Pointer to the Parser instance
     * @param[in]  buf  NULL terminated character string with the command to be parsed
     * @return PARSER_ERR_OK on success, PARSER_ERR_NULL_ARG / PARSER_ERR_BUF_TOO_LONG / PARSER_ERR_BUF_EMPTY /
     * PARSER_ERR_CMD_INCOMPLETE / PARSER_ERR_TOKEN_TOO_LONG / PARSER_ERR_PTHREAD_FAILURE otherwise
     * @note Parser will associate the parsed buf with the first cmd from the list that matches target and action
     */
    ParserError_t (*execute)(struct Parser* ctx, const char* buf);

    /**
     * @brief Deinit the parser (destroy the mutex)
     * @param[in, out]  ctx  Pointer to the Parser instance
     * @return PARSER_ERR_OK on success, PARSER_ERR_PTHREAD_FAILURE otherwise
     */
    ParserError_t (*deinit)(struct Parser* ctx);
} Parser_t;

/**
 * @brief Initialize a new Parser instance
 * @param[in, out]  ctx  Pointer to the Parser instance
 * @param[in]  id  ID of the command to be removed
 * @return PARSER_ERR_OK on success, PARSER_ERR_NULL_ARG or PARSER_ERR_PTHREAD_FAILURE otherwise
 */
ParserError_t parser_init(Parser_t* ctx, const ParserConfig_t cfg);

#endif // __CMD_PARSER_H__