#include "utils/log.h"

#include <pthread.h>
#include <stdarg.h>      // For: variadic function utils
#include <stdio.h>       // For: sprintf, vsprintf etc.
#include <string.h>      // For: strrchr()
#include <sys/syscall.h> // For: syscall()
#include <sys/types.h>   // For: struct tm
#include <time.h>        // For: strftime, time(), localtime(), time_t etc.
#include <unistd.h>

#define GET_THREAD_ID() ((long)syscall(SYS_gettid))

// Set default logging level if not defined
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#endif

// Set the format in which time is printed in logs
#define LOG_TIME_FORMAT "[%H:%M:%S]"

void log_print(const char* level, const int level_num, const char* file, const int line, const char* msg, ...) {
    va_list args;

    if(LOG_LEVEL <= level_num) {
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        char time_str[20];
        strftime(time_str, sizeof(time_str), LOG_TIME_FORMAT, tm_info);

        const char* filename = strrchr(file, '/');
        filename = filename ? filename + 1 : file;

        long tid = GET_THREAD_ID();

        va_start(args, msg);
        fprintf(LOG_OUTPUT, "%s [TID:%ld] %s %s:%d: ", time_str, tid, level, filename, line);
        vfprintf(LOG_OUTPUT, msg, args);
        fprintf(LOG_OUTPUT, "\n");
        va_end(args);
    }
}
