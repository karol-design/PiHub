/* Should be thread-safe (currently is not) */

#include <stdio.h>

#define log_info(msg, ...)                           \
    {                                                \
        printf("INFO: %s:%d: ", __FILE__, __LINE__); \
        printf((msg), ##__VA_ARGS__);                \
        printf("\n");                                \
    }

#define log_error(msg, ...)                           \
    {                                                 \
        printf("ERROR: %s:%d: ", __FILE__, __LINE__); \
        printf((msg), ##__VA_ARGS__);                 \
        printf("\n");                                 \
    }
