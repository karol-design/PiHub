#include "app/dispatcher.h"

#include <string.h> // For: strtok_r(), strnlen()

#include "utils/common.h"
#include "utils/log.h"

typedef struct {
    char target[DISPATCHER_TARGET_MAX_SIZE];                 // E.g. "gpio", "sensor", "server"
    char action[DISPATCHER_ACTION_MAX_SIZE];                 // E.g. "set", "get", "status"
    char argv[DISPATCHER_MAX_ARGS][DISPATCHER_ARG_MAX_SIZE]; // Additional parameters (e.g. GPIO Pin number, sensor ID)
    uint32_t argc;                                           // Number of arguments
} TokenizedCommand_t;

/**
 * @brief Tokenize the buffer with the command
 * @param[in]  buf  Pointer to the character string to be parsed [must be NULL-terminated]
 * @param[in]  delim  Delimiter string
 * @param[in, out]  output  Pointer to the TokenizedCommand_t struct where results will be saved
 * @return DISPATCHER_ERR_OK on success, DISPATCHER_ERR_NULL_ARG / DISPATCHER_ERR_BUF_TOO_LONG /
 * DISPATCHER_ERR_BUF_EMPTY / DISPATCHER_ERR_CMD_INCOMPLETE / DISPATCHER_ERR_TOKEN_TOO_LONG otherwise
 */
STATIC DispatcherError_t dispatcher_tokenize(const char* buf, const char* delim, TokenizedCommand_t* output);

STATIC DispatcherError_t dispatcher_tokenize(const char* buf, const char* delim, TokenizedCommand_t* output) {
    if(!buf || !output) {
        return DISPATCHER_ERR_NULL_ARG;
    }

    size_t buf_len = strnlen(buf, DISPATCHER_MAX_BUF_SIZE - 1);
    if(buf_len >= DISPATCHER_MAX_BUF_SIZE) {
        return DISPATCHER_ERR_BUF_TOO_LONG;
    }

    DispatcherError_t err = DISPATCHER_ERR_OK;
    char *token, *next;
    char _buf[DISPATCHER_MAX_BUF_SIZE];
    memcpy(_buf, buf, buf_len + 1);
    size_t token_len;
    memset(output, 0, sizeof(TokenizedCommand_t)); // Zero-out the struct for safety

    // The first token must represent the target of the command
    token = strtok_r(_buf, delim, &next); // Use strtok_r for thread-safe behaviour
    if(!token) {
        return DISPATCHER_ERR_BUF_EMPTY;
    }
    token_len = strnlen(token, DISPATCHER_TARGET_MAX_SIZE - 1);
    if(token_len >= DISPATCHER_TARGET_MAX_SIZE) {
        log_error("'target' token too long (len: %lu)", token_len);
        return DISPATCHER_ERR_TOKEN_TOO_LONG;
    }
    memcpy(output->target, token, token_len + 1);
    output->target[DISPATCHER_TARGET_MAX_SIZE - 1] = '\0'; // Cap 'target' with NULL char

    // The second token must represent the action to be performed
    token = strtok_r(next, delim, &next);
    if(!token) {
        return DISPATCHER_ERR_CMD_INCOMPLETE;
    }
    token_len = strnlen(token, DISPATCHER_ACTION_MAX_SIZE - 1);
    if(token_len >= DISPATCHER_ACTION_MAX_SIZE) {
        log_error("'action' token too long (len: %lu)", token_len);
        return DISPATCHER_ERR_TOKEN_TOO_LONG;
    }
    memcpy(output->action, token, token_len + 1);
    output->action[DISPATCHER_ACTION_MAX_SIZE - 1] = '\0'; // Cap 'action' with NULL char

    // Target and Action should be followed by parameters
    for(uint32_t arg_no = 0; arg_no < DISPATCHER_MAX_ARGS; arg_no++) {
        token = strtok_r(next, delim, &next);
        if(!token) {
            break; // No other arguments (parameters) can be found
        }
        token_len = strnlen(token, DISPATCHER_ARG_MAX_SIZE - 1);
        if(token_len >= DISPATCHER_ARG_MAX_SIZE) {
            log_error("one of 'argument' tokens is too long (len: %lu)", token_len);
            return DISPATCHER_ERR_TOKEN_TOO_LONG;
        }
        memcpy(output->argv[arg_no], token, token_len + 1);
        output->argv[arg_no][DISPATCHER_ARG_MAX_SIZE - 1] = '\0'; // Cap 'arg' with NULL char
        output->argc++;
    }

    if(*next != '\0') {
        err = DISPATCHER_ERR_TOO_MANY_ARGS;
    }

    return err;
}

DispatcherError_t dispatcher_init(Dispatcher_t* ctx, const DispatcherConfig_t cfg) {
    if(!ctx || !cfg.delim) {
        return DISPATCHER_ERR_NULL_ARG;
    } else if(strnlen(cfg.delim, DISPATCHER_MAX_DELIM_SIZE - 1) >= DISPATCHER_MAX_DELIM_SIZE - 1) {
        return DISPATCHER_ERR_DELIM_TOO_LONG;
    }

    // Zero-out the Dispatcher_t struct and its members on init
    memset(ctx, 0, sizeof(Dispatcher_t));

    // Initialize mutex for protecting dispatcher-related critical sections
    int ret = pthread_mutex_init(&ctx->lock, NULL);
    if(ret != 0) {
        log_error("pthread_mutex_init() returned %d", ret);
        return DISPATCHER_ERR_PTHREAD_FAILURE;
    }

    // Populate data in the struct (cfg) and assign function pointers
    ctx->cfg = cfg;

    return DISPATCHER_ERR_OK;
}

DispatcherError_t dispatcher_register(Dispatcher_t* ctx, const uint32_t id, const DispatcherCommandDef_t cmd) {
    if(!ctx || !cmd.target || !cmd.action || !cmd.callback_ptr) {
        return DISPATCHER_ERR_NULL_ARG;
    } else if(id >= DISPATCHER_MAX_CMD_COUNT) {
        return DISPATCHER_ERR_INVALID_ARG;
    } else if(strlen(cmd.action) == 0 || strlen(cmd.target) == 0) {
        return DISPATCHER_ERR_INVALID_ARG;
    }
    DispatcherError_t err = DISPATCHER_ERR_OK;

    // Add a new command to the Dispatcher (critical section)
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return DISPATCHER_ERR_PTHREAD_FAILURE;
    }
    log_debug("dispatcher lock taken");

    if(ctx->cmd_list[id].valid) {
        err = DISPATCHER_ERR_ID_ALREADY_TAKEN;
    } else {
        ctx->cmd_list[id].cfg = cmd;
        ctx->cmd_list[id].valid = true;
    }

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return DISPATCHER_ERR_PTHREAD_FAILURE;
    }
    log_debug("dispatcher lock released");

    return err;
}

DispatcherError_t dispatcher_deregister(Dispatcher_t* ctx, const uint32_t id) {
    if(!ctx) {
        return DISPATCHER_ERR_NULL_ARG;
    } else if(id >= DISPATCHER_MAX_CMD_COUNT) {
        return DISPATCHER_ERR_INVALID_ARG;
    }

    // Remove a command definition from the Dispatcher (critical section)
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return DISPATCHER_ERR_PTHREAD_FAILURE;
    }
    log_debug("dispatcher lock taken");

    ctx->cmd_list[id].valid = false;

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return DISPATCHER_ERR_PTHREAD_FAILURE;
    }
    log_debug("dispatcher lock released");

    return DISPATCHER_ERR_OK;
}

DispatcherError_t dispatcher_execute(Dispatcher_t* ctx, const char* buf, const void* cmd_ctx) {
    if(!ctx || !buf) {
        return DISPATCHER_ERR_NULL_ARG;
    } else if(strnlen(buf, DISPATCHER_MAX_BUF_SIZE) >= DISPATCHER_MAX_BUF_SIZE) {
        return DISPATCHER_ERR_BUF_TOO_LONG;
    }

    TokenizedCommand_t tokens;
    DispatcherError_t err = dispatcher_tokenize(buf, ctx->cfg.delim, &tokens);
    if(err != DISPATCHER_ERR_OK) {
        return err;
    }

    // Itterate through the cmd list and look for the first cmd that will match target & action (critical section)
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return DISPATCHER_ERR_PTHREAD_FAILURE;
    }
    log_debug("dispatcher lock taken");

    bool match = false;
    for(uint32_t cmd_no = 0; cmd_no < DISPATCHER_MAX_CMD_COUNT; cmd_no++) {
        if(ctx->cmd_list[cmd_no].valid) {
            // Compare target and action (string-insensitive)
            match = !strncasecmp(tokens.target, ctx->cmd_list[cmd_no].cfg.target, DISPATCHER_TARGET_MAX_SIZE);
            match &= !strncasecmp(tokens.action, ctx->cmd_list[cmd_no].cfg.action, DISPATCHER_ACTION_MAX_SIZE);
            if(match) {
                // Invoke the callback associated with the parsed command
                if(ctx->cmd_list[cmd_no].cfg.callback_ptr == NULL) {
                    err = DISPATCHER_ERR_NULL_ARG;
                    break;
                }
                ctx->cmd_list[cmd_no].cfg.callback_ptr((char*)tokens.argv, tokens.argc, cmd_ctx);
                break; // Finish the search on the first cmd that matched
            }
        }
    }

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return DISPATCHER_ERR_PTHREAD_FAILURE;
    }
    log_debug("dispatcher lock released");

    if(!match && err != DISPATCHER_ERR_NULL_ARG) {
        err = DISPATCHER_ERR_CMD_NOT_FOUND;
    }

    return err;
}

DispatcherError_t dispatcher_deinit(Dispatcher_t* ctx) {
    if(!ctx) {
        return DISPATCHER_ERR_NULL_ARG;
    }
    // Destroy the lock (@TODO: Improve mutex destroy mechanism, e.g. add a global protection variable)
    int ret = pthread_mutex_destroy(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_destroy() returned %d", ret);
        return DISPATCHER_ERR_PTHREAD_FAILURE;
    }

    // Zero-out the Dispatcher_t struct along with its members on deinit
    memset(ctx, 0, sizeof(Dispatcher_t));

    return DISPATCHER_ERR_OK;
}
