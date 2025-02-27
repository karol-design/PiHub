#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define GET_THREAD_ID() GetCurrentThreadId()
#else
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#define GET_THREAD_ID() ((long)syscall(SYS_gettid))
#endif

// Define logging levels
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_ERROR 2

// Set default logging level if not defined
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

#define LOG_TIME_FORMAT "[%H:%M:%S]"

#define LOG_PRINT(level, level_num, msg, ...)                                            \
    do {                                                                                 \
        if(LOG_LEVEL <= level_num) {                                                     \
            time_t now = time(NULL);                                                     \
            struct tm* tm_info = localtime(&now);                                        \
            char time_str[20];                                                           \
            strftime(time_str, sizeof(time_str), LOG_TIME_FORMAT, tm_info);              \
            const char* filename = strrchr(__FILE__, '/');                               \
            filename = filename ? filename + 1 : __FILE__;                               \
            long tid = GET_THREAD_ID();                                                  \
            printf("%s [TID:%ld] %s %s:%d: ", time_str, tid, level, filename, __LINE__); \
            printf((msg), ##__VA_ARGS__);                                                \
            printf("\n");                                                                \
        }                                                                                \
    } while(0)

#define log_debug(msg, ...) LOG_PRINT("DEBUG", LOG_LEVEL_DEBUG, msg, ##__VA_ARGS__)
#define log_info(msg, ...) LOG_PRINT("INFO", LOG_LEVEL_INFO, msg, ##__VA_ARGS__)
#define log_error(msg, ...) LOG_PRINT("ERROR", LOG_LEVEL_ERROR, msg, ##__VA_ARGS__)
