/**
 * @file log.h
 * @brief Simple logging library with levels, date/time/thread/file info and output redirection
 *
 * @note Designed to provide thread-safe functionality (MT-Safe)
 *
 */

#ifndef __LOG_H__
#define __LOG_H__

// Define logging levels
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_ERROR 2

// Configure where the logs should be printed
#define LOG_OUTPUT stdout

#ifdef LOGS_ENABLED

#define log_debug(msg, ...) log_print("DEBUG", LOG_LEVEL_DEBUG, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define log_info(msg, ...) log_print("INFO", LOG_LEVEL_INFO, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define log_error(msg, ...) log_print("ERROR", LOG_LEVEL_ERROR, __FILE__, __LINE__, msg, ##__VA_ARGS__)

#else

// Ignore all logs if disabled
#define log_debug(msg, ...)
#define log_info(msg, ...)
#define log_error(msg, ...)

#endif // LOGS_ENABLED

void log_print(const char* level, const int level_num, const char* file, const int line, const char* msg, ...);

#endif // __LOG_H__