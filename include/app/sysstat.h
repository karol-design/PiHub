/**
 * @file sysstat.h
 * @brief A component for retrieving and parsing host operating system statistics.
 *
 */

#ifndef __SYSSTAT_H__
#define __SYSSTAT_H__

#include <stdint.h> // For: std integer types

/**
 * @enum SysstatError_t
 * @brief Error codes returned by Sysstat API functions
 */
typedef enum {
    SYSSTAT_ERR_OK = 0x00,          /**< Operation finished successfully */
    SYSSTAT_ERR_NULL_ARGUMENT,      /**< Error: NULL ptr passed as argument */
    SYSSTAT_ERR_FILESYSTEM_FAILURE, /**< Error: File open/read/write/close operation failure */
    SYSSTAT_ERR_FILE_EMPTY,         /**< Error: Linux Kernel file is empty */
    SYSSTAT_ERR_BUF_TOO_SHORT,      /**< Error: The input buffer is too short */
    SYSSTAT_ERR_GENERIC,            /**< Error: Generic error */
} SysstatError_t;

/**
 * @struct SysstatMemInfo_t
 * @brief Stores parsed memory information from /proc/meminfo
 */
typedef struct {
    uint64_t total_kB;     // Total usable RAM
    uint64_t free_kB;      // Completely unused RAM
    uint64_t available_kB; // Estimated available mem for new apps (free + cashe/buffer)
} SysstatMemInfo_t;

/**
 * @struct SysstatNetInfo_t
 * @brief Stores network statistics for a single interface, parsed from /proc/net/dev
 */
typedef struct {
    uint64_t rx_bytes;
    uint64_t rx_packets;
    uint64_t tx_bytes;
    uint64_t tx_packets;
} SysstatNetInfo_t;

typedef struct {
    uint32_t s;
    uint16_t ms;
} SysstatTime_t;

typedef struct {
    SysstatTime_t up;
    SysstatTime_t idle;
} SysstatUptimeInfo_t;

/**
 * @brief Retrieves the system uptime in seconds from /proc/uptime
 *
 * @param[out] uptime_sec Pointer to store uptime value
 * @return SysstatError_t Error code indicating success or failure
 */
SysstatError_t sysstat_get_uptime_info(SysstatUptimeInfo_t* uptime);

/**
 * @brief Retrieves memory information
 *
 * @param[out] mem_info Pointer to SysstatMemInfo_t to store results
 * @return SysstatError_t Error code indicating success or failure
 */
SysstatError_t sysstat_get_mem_info(SysstatMemInfo_t* mem_info);

/**
 * @brief Retrieves network information for a specific interface
 *
 * @param[in] interface_name Name of the network interface (e.g., "eth0")
 * @param[out] net_info Pointer to SysstatNetInfo_t to store results
 * @return SysstatError_t Error code indicating success or failure
 */
SysstatError_t sysstat_get_net_info(const char* interface_name, SysstatNetInfo_t* net_info);

#endif // __SYSSTAT_H__
