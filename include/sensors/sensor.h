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
#include <stdbool.h> // For: boolean types
#include <stdint.h>  // For: std types

#include "hw/hw_interface.h"
#include "hw/i2c_bus.h"
#include "hw/spi_bus.h"

/**
 * @struct SensorError_t
 * @brief Error codes returned by sensor API functions
 */
typedef enum {
    SENSOR_ERR_OK = 0x00,        /**< Operation finished successfully */
    SENSOR_ERR_NULL_ARGUMENT,    /**< Error: NULL ptr passed as argument */
    SENSOR_ERR_OP_NOT_SUPPORTED, /**< Error: Operation not supported (e.g. reading hum on temp-only sensor) */
    SENSOR_ERR_SENSOR_NOT_SUPPORTED,      /**< Error: Sensor model is not supported yet */
    SENSOR_ERR_HW_INTERFACE_INIT_FAILURE, /**< Error: Hardware interface (e.g. I2C/SPI) init failure */
    SENSOR_ERR_HW_INTERFACE_FAILURE,      /**< Error: Hardware interface operation failure */
    SENSOR_ERR_NOT_INITIALIZED,           /**< Error: Sensor not initialized yet */
    SENSOR_ERR_NOT_RESPONDING,            /**< Error: Sensor is not responding */
    SENSOR_ERR_PTHREAD_FAILURE,           /**< Error: Pthread API call failure */
    SENSOR_ERR_MALLOC_FAILURE,            /**< Error: Malloc call failure */
    SENSOR_ERR_GENERIC,                   /**< Error: Generic error */
} SensorError_t;

/**
 * @struct SensorOutput_t
 * @brief Union with the result of the measurement
 */
typedef union {
    float temp;  // Temperature in degrees Celcius
    float hum;   // Humidity in percentage
    float press; // Pressure in hPa
} SensorOutput_t;

/**
 * @struct SensorMeasurement_t
 * @brief Types of measurements that can be retrieved from the sensor
 */
typedef enum {
    SENSOR_MEASUREMENT_TEMP = 0x00, /**< Measurement: Temperature */
    SENSOR_MEASUREMENT_HUM,         /**< Measurement: Humidity */
    SENSOR_MEASUREMENT_PRESS,       /**< Measurement: Pressure */
} SensorMeasurement_t;

/**
 * @struct Sensor_t
 * @brief Include configuration data, mutex for thread-safety, and pointers to member functions.
 */
typedef struct Sensor {
    uint8_t addr;         // Address of the sensor (7 lower bits for I2C / CS GPIO pin for SPI)
    HwInterface_t hw_ctx; // Hardware interface context
    bool is_initialized;  // Init flag

    SensorError_t (*get_reading)(struct Sensor* ctx, const SensorMeasurement_t type, SensorOutput_t* out);
    SensorError_t (*get_status)(struct Sensor* ctx);
    SensorError_t (*deinit)(struct Sensor* ctx);
} Sensor_t;

#endif // __SENSOR_H__