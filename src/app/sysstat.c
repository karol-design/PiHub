#include "app/sysstat.h"

#include <errno.h>  // for: errno
#include <stdio.h>  // for: fopen, strerror
#include <string.h> // For: strtok_r(), strnlen()

#include "utils/common.h"
#include "utils/log.h"

#define SYSSTAT_UPTIME_PATH "/proc/uptime"
#define SYSSTAT_MEMINFO_PATH "/proc/meminfo"
#define SYSSTAT_NET_DEV_PATH "/proc/net/dev"

#define SYSSTAT_UPTIME_BUF_LEN 40
#define SYSSTAT_MEMINFO_BUF_LEN 2048
#define SYSSTAT_NET_DEV_BUF_LEN 1024

/**
 * @brief Read all characters from the file into the buffer
 *
 * @param[in, out] buf Pointer to the buffer where file contents will be stored
 * @param[in] buf_len Length of the buffer, including space for null terminator
 * @param[in] path Path to the file to be read
 * @return SYSSTAT_ERR_OK on success, SYSSTAT_ERR_NULL_ARGUMENT / SYSSTAT_ERR_BUF_TOO_SHORT / SYSSTAT_ERR_FILESYSTEM_FAILURE otherwise
 */
STATIC SysstatError_t file_to_buf(char* buf, const size_t buf_len, const char* path) {
    if(!buf || !path) {
        return SYSSTAT_ERR_NULL_ARGUMENT;
    } else if(buf_len <= 1) {
        return SYSSTAT_ERR_BUF_TOO_SHORT;
    }

    // Open the file for reading
    FILE* f_ptr = fopen(path, "r");
    if(!f_ptr) {
        log_error("failed to open %.25s (errno: %s)", path, strerror(errno));
        return SYSSTAT_ERR_FILESYSTEM_FAILURE;
    }

    char* end = buf + buf_len - 1; // Reserve one byte for the null terminator
    int c;

    // Read all characters one by one
    while((c = getc(f_ptr)) != EOF && buf < end) {
        *buf = (char)c;
        buf++;
    }
    *buf = '\0';

    // Close the file once the data has been saved in the buffer
    int ret = fclose(f_ptr);
    if(ret != 0) {
        log_error("failed to open %.25s (errno: %s)", path, strerror(errno));
        return SYSSTAT_ERR_FILESYSTEM_FAILURE;
    }

    // Return error if the file still has characters but the buffer is already full
    if(c != EOF && buf == end) {
        return SYSSTAT_ERR_BUF_TOO_SHORT;
    }

    return SYSSTAT_ERR_OK;
}

SysstatError_t sysstat_get_uptime_info(SysstatUptimeInfo_t* uptime) {
    if(!uptime) {
        return SYSSTAT_ERR_NULL_ARGUMENT;
    }

    // Read the /uptime file into the buffer
    char buf[SYSSTAT_UPTIME_BUF_LEN];
    SysstatError_t err = file_to_buf(buf, SYSSTAT_UPTIME_BUF_LEN, SYSSTAT_UPTIME_PATH);
    if(err != SYSSTAT_ERR_OK) {
        return err;
    }

    // @TODO: sscanf should be replaced with saver tokenizer & strto...() to prevent conversion overflow
    if(sscanf(buf, "%u.%hu %u.%hu", &uptime->up.s, &uptime->up.ms, &uptime->idle.s, &uptime->idle.ms) != 4) {
        return SYSSTAT_ERR_GENERIC;
    }

    return SYSSTAT_ERR_OK;
}

SysstatError_t sysstat_get_mem_info(SysstatMemInfo_t* mem_info) {
    if(!mem_info) {
        return SYSSTAT_ERR_NULL_ARGUMENT;
    }

    // Read the /proc/meminfo file into the buffer
    char buf[SYSSTAT_MEMINFO_BUF_LEN];
    SysstatError_t err = file_to_buf(buf, SYSSTAT_MEMINFO_BUF_LEN, SYSSTAT_MEMINFO_PATH);
    if(err != SYSSTAT_ERR_OK) {
        return err;
    }

    // @TODO: sscanf should be replaced with saver tokenizer & strto...() to prevent conversion errors (e.g. overflow or generic failure)
    if(sscanf(buf, "MemTotal: %lu kB MemFree: %lu kB MemAvailable: %lu", &mem_info->total_kB,
       &mem_info->free_kB, &mem_info->available_kB) == EOF) {
        return SYSSTAT_ERR_GENERIC;
    }

    return SYSSTAT_ERR_OK;
}

SysstatError_t sysstat_get_net_info(const char* interface_name, SysstatNetInfo_t* net_info) {
    if(!net_info || !interface_name) {
        return SYSSTAT_ERR_NULL_ARGUMENT;
    }

    // Read the /proc/net/dev file into the buffer
    char buf[SYSSTAT_NET_DEV_BUF_LEN];
    SysstatError_t err = file_to_buf(buf, SYSSTAT_NET_DEV_BUF_LEN, SYSSTAT_NET_DEV_PATH);
    if(err != SYSSTAT_ERR_OK) {
        return err;
    }

    // Move to the line with the interface-of-interest statistics
    char* if_stats = strstr(buf, interface_name);
    if(!if_stats) {
        return SYSSTAT_ERR_GENERIC;
    }

    // @TODO: sscanf should be replaced with saver tokenizer & strto...() to prevent conversion errors (e.g. overflow or generic failure)
    if(sscanf(if_stats, "%*32s %lu %lu %*u %*u %*u %*u %*u %*u %lu %lu", &net_info->rx_bytes,
       &net_info->rx_packets, &net_info->tx_bytes, &net_info->tx_packets) != 4) {
        return SYSSTAT_ERR_GENERIC;
    }

    return SYSSTAT_ERR_OK;
}