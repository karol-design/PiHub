/**
 * @file sensor.h
 * @brief A simple API for environmental sensors [intended to be easily extended, to other sensor types (e.g. IMU), if necessary]
 *
 * @note Designed to provide thread-safe functionality (MT-Safe)
 *
 * @note Use sensor_init(), and sensor.deinit() to initialize and deinitialize new Sensor instance. Use: ...
 */

#ifndef __SENSOR_H__
#define __SENSOR_H__

#include <pthread.h> // For: pthread_mutex_t and related functions
#include <stdint.h>  // For: std types

/**
 * @struct SensorError_t
 * @brief Error codes returned by sensor API functions
 */
typedef enum {
    SENSOR_ERR_OK = 0x00,        /**< Operation finished successfully */
    SENSOR_ERR_NULL_ARGUMENT,    /**< Error: NULL ptr passed as argument */
    SENSOR_ERR_OP_NOT_SUPPORTED, /**< Error: Operation not supported (e.g. reading hum on temp-only sensor) */
    SENSOR_ERR_PTHREAD_FAILURE,  /**< Error: Pthread API call failure */
    SENSOR_ERR_HW_INTERFACE_FAILURE, /**< Error: Hardware interface (e.g. I2C/SPI) failure */
    SENSOR_ERR_GENERIC,              /**< Error: Generic error */
} SensorError_t;

/**
 * @struct SensorMeasType_t
 * @brief Possible measurement categories (intended to be logically ORed)
 */
typedef enum {
    SENSOR_ = 0x00,
    SENSOR_TEMP = 0x01, /**< Sensor can measure temperature */
    SENSOR_HUM = 0x02,  /**< Sensor can measure humidity */
    SENSOR_PRESS = 0x04 /**< Sensor can measure pressure */
} SensorMeasType_t;

typedef struct {
    uint16_t type; // Supported measurement categories (logical OR of relevant members of SensorMeasType_t)
} SensorConfig_t;

/**
 * @struct Sensor_t
 * @brief Include configuration data, mutex for thread-safety, and pointers to member functions.
 */
typedef struct Sensor {
    SensorConfig_t cfg;   // Sensor config including ...
    pthread_mutex_t lock; // Lock for sensor-related critical sections
} Sensor_t;

/**
 * @brief Initialize a new Sensor instance
 * @param[in, out]  ctx  Pointer to the Sensor instance
 * @param[in]  cfg  Configuration structure
 * @return SENSOR_ERR_OK on success, SENSOR_ERR_NULL_ARG or SENSOR_ERR_PTHREAD_FAILURE otherwise
 */
SensorError_t sensor_init(Sensor_t* ctx, const SensorConfig_t cfg);

SensorError_t sensor_get_temp(Sensor_t* ctx, float* temp); // @TODO: Temp format TBD
SensorError_t get_hum(Sensor_t* ctx, float* hum);          // @TODO: Hum format TBD
SensorError_t get_press(Sensor_t* ctx, float* press);      // @TODO: Press format TBD
SensorError_t get_status(Sensor_t* ctx);
SensorError_t deinit(Sensor_t* ctx);

#endif // __SENSOR_H__