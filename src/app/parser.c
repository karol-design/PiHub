#include "app/parser.h"

#include <string.h> // For: strtok_r(), strnlen()

#include "utils/common.h"
#include "utils/log.h"

typedef struct {
    char target[PARSER_TARGET_MAX_SIZE];             // E.g. "gpio", "sensor", "server"
    char action[PARSER_ACTION_MAX_SIZE];             // E.g. "set", "get", "status"
    char argv[PARSER_MAX_ARGS][PARSER_ARG_MAX_SIZE]; // Additional parameters (e.g. GPIO Pin number, sensor ID)
    uint32_t argc;                                   // Number of arguments
} TokenizedCommand_t;

STATIC ParserError_t parser_add_cmd(Parser_t* ctx, const uint32_t id, const ParserCommandDef_t cmd);
STATIC ParserError_t parser_remove_cmd(Parser_t* ctx, const uint32_t id);
STATIC ParserError_t parser_execute(Parser_t* ctx, const char* buf);
STATIC ParserError_t parser_deinit(Parser_t* ctx);

/**
 * @brief Tokenize the buffer with the command
 * @param[in]  buf  Pointer to the character string to be parsed [must be NULL-terminated]
 * @param[in]  delim  Delimiter string
 * @param[in, out]  output  Pointer to the TokenizedCommand_t struct where results will be saved
 * @return PARSER_ERR_OK on success, PARSER_ERR_NULL_ARG / PARSER_ERR_BUF_TOO_LONG /
 * PARSER_ERR_BUF_EMPTY / PARSER_ERR_CMD_INCOMPLETE / PARSER_ERR_TOKEN_TOO_LONG otherwise
 */
STATIC ParserError_t parser_tokenize(const char* buf, const char* delim, TokenizedCommand_t* output);

STATIC ParserError_t parser_tokenize(const char* buf, const char* delim, TokenizedCommand_t* output) {
    if(!buf || !output) {
        return PARSER_ERR_NULL_ARG;
    }

    size_t buf_len = strnlen(buf, PARSER_MAX_BUF_SIZE - 1);
    if(buf_len >= PARSER_MAX_BUF_SIZE) {
        return PARSER_ERR_BUF_TOO_LONG;
    }

    ParserError_t err = PARSER_ERR_OK;
    char *token, *next;
    char _buf[PARSER_MAX_BUF_SIZE];
    memcpy(_buf, buf, buf_len + 1);
    size_t token_len;
    memset(output, 0, sizeof(TokenizedCommand_t)); // Zero-out the struct for safety

    // The first token must represent the target of the command
    token = strtok_r(_buf, delim, &next); // Use strtok_r for thread-safe behaviour
    if(!token) {
        return PARSER_ERR_BUF_EMPTY;
    }
    token_len = strnlen(token, PARSER_TARGET_MAX_SIZE - 1);
    if(token_len >= PARSER_TARGET_MAX_SIZE) {
        log_error("'target' token too long (len: %lu)", token_len);
        return PARSER_ERR_TOKEN_TOO_LONG;
    }
    memcpy(output->target, token, token_len + 1);
    output->target[PARSER_TARGET_MAX_SIZE - 1] = '\0'; // Cap 'target' with NULL char

    // The second token must represent the action to be performed
    token = strtok_r(next, delim, &next);
    if(!token) {
        return PARSER_ERR_CMD_INCOMPLETE;
    }
    token_len = strnlen(token, PARSER_ACTION_MAX_SIZE - 1);
    if(token_len >= PARSER_ACTION_MAX_SIZE) {
        log_error("'action' token too long (len: %lu)", token_len);
        return PARSER_ERR_TOKEN_TOO_LONG;
    }
    memcpy(output->action, token, token_len + 1);
    output->action[PARSER_ACTION_MAX_SIZE - 1] = '\0'; // Cap 'action' with NULL char

    // Target and Action should be followed by parameters
    for(uint32_t arg_no = 0; arg_no < PARSER_MAX_ARGS; arg_no++) {
        token = strtok_r(next, delim, &next);
        if(!token) {
            break; // No other arguments (parameters) can be found
        }
        token_len = strnlen(token, PARSER_ARG_MAX_SIZE - 1);
        if(token_len >= PARSER_ARG_MAX_SIZE) {
            log_error("one of 'argument' tokens is too long (len: %lu)", token_len);
            return PARSER_ERR_TOKEN_TOO_LONG;
        }
        memcpy(output->argv[arg_no], token, token_len + 1);
        output->argv[arg_no][PARSER_ARG_MAX_SIZE - 1] = '\0'; // Cap 'arg' with NULL char
        output->argc++;
    }

    if(*next != '\0') {
        err = PARSER_ERR_TOO_MANY_ARGS;
    }

    return err;
}

ParserError_t parser_add_cmd(Parser_t* ctx, const uint32_t id, const ParserCommandDef_t cmd) {
    if(!ctx || !cmd.target || !cmd.action || !cmd.callback_ptr) {
        return PARSER_ERR_NULL_ARG;
    } else if(id >= PARSER_MAX_CMD_COUNT) {
        return PARSER_ERR_INVALID_ARG;
    }
    ParserError_t err = PARSER_ERR_OK;

    // Add a new command to the Parser (critical section)
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return PARSER_ERR_PTHREAD_FAILURE;
    }
    log_debug("parser lock taken");

    if(ctx->cmd_list[id].valid) {
        err = PARSER_ERR_ID_ALREADY_TAKEN;
    } else {
        ctx->cmd_list[id].cfg = cmd;
        ctx->cmd_list[id].valid = true;
    }

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return PARSER_ERR_PTHREAD_FAILURE;
    }
    log_debug("parser lock released");

    return err;
}

ParserError_t parser_remove_cmd(Parser_t* ctx, const uint32_t id) {
    if(!ctx) {
        return PARSER_ERR_NULL_ARG;
    } else if(id >= PARSER_MAX_CMD_COUNT) {
        return PARSER_ERR_INVALID_ARG;
    }

    // Remove a command definition from the Parser (critical section)
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return PARSER_ERR_PTHREAD_FAILURE;
    }
    log_debug("parser lock taken");

    ctx->cmd_list[id].valid = false;

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return PARSER_ERR_PTHREAD_FAILURE;
    }
    log_debug("parser lock released");

    return PARSER_ERR_OK;
}

ParserError_t parser_execute(Parser_t* ctx, const char* buf) {
    if(!ctx || !buf) {
        return PARSER_ERR_NULL_ARG;
    } else if(strnlen(buf, PARSER_MAX_BUF_SIZE) >= PARSER_MAX_BUF_SIZE) {
        return PARSER_ERR_BUF_TOO_LONG;
    }

    TokenizedCommand_t tokens;
    ParserError_t err = parser_tokenize(buf, ctx->cfg.delim, &tokens);
    if(err != PARSER_ERR_OK) {
        return err;
    }

    // Itterate through the cmd list and look for the first cmd that will match target & action (critical section)
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return PARSER_ERR_PTHREAD_FAILURE;
    }
    log_debug("parser lock taken");

    bool match = false;
    for(uint32_t cmd_no = 0; cmd_no < PARSER_MAX_CMD_COUNT; cmd_no++) {
        if(ctx->cmd_list[cmd_no].valid) {
            // Compare target and action (string-insensitive)
            match = !strncasecmp(tokens.target, ctx->cmd_list[cmd_no].cfg.target, PARSER_TARGET_MAX_SIZE);
            match &= !strncasecmp(tokens.action, ctx->cmd_list[cmd_no].cfg.action, PARSER_ACTION_MAX_SIZE);
            if(match) {
                // Invoke the callback associated with the parsed command
                if(ctx->cmd_list[cmd_no].cfg.callback_ptr == NULL) {
                    err = PARSER_ERR_NULL_ARG;
                    break;
                }
                ctx->cmd_list[cmd_no].cfg.callback_ptr((char*)tokens.argv, tokens.argc);
                break; // Finish the search on the first cmd that matched
            }
        }
    }

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return PARSER_ERR_PTHREAD_FAILURE;
    }
    log_debug("parser lock released");

    if(!match && err != PARSER_ERR_NULL_ARG) {
        err = PARSER_ERR_CMD_NOT_FOUND;
    }

    return err;
}

ParserError_t parser_deinit(Parser_t* ctx) {
    // Destroy the lock (@TODO: Improve mutex destroy mechanism, e.g. add a global protection variable)
    int ret = pthread_mutex_destroy(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_destroy() returned %d", ret);
        return PARSER_ERR_PTHREAD_FAILURE;
    }

    // Zero-out the Parser_t struct and its members on deinit
    memset(ctx, 0, sizeof(Parser_t));

    return PARSER_ERR_OK;
}

ParserError_t parser_init(Parser_t* ctx, const ParserConfig_t cfg) {
    if(!ctx || !cfg.delim) {
        return PARSER_ERR_NULL_ARG;
    } else if(strnlen(cfg.delim, PARSER_MAX_DELIM_SIZE - 1) >= PARSER_MAX_DELIM_SIZE - 1) {
        return PARSER_ERR_DELIM_TOO_LONG;
    }

    // Zero-out the Parser_t struct and its members on init
    memset(ctx, 0, sizeof(Parser_t));

    // Initialize mutex for protecting parser-related critical sections
    int ret = pthread_mutex_init(&ctx->lock, NULL);
    if(ret != 0) {
        log_error("pthread_mutex_init() returned %d", ret);
        return PARSER_ERR_PTHREAD_FAILURE;
    }

    // Populate data in the struct (cfg) and assign function pointers
    ctx->cfg = cfg;
    ctx->add_cmd = parser_add_cmd;
    ctx->remove_cmd = parser_remove_cmd;
    ctx->execute = parser_execute;
    ctx->deinit = parser_deinit;

    return PARSER_ERR_OK;
}
