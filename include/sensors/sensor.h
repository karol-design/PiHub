/**
 * @file sensor.h
 * @brief Typedefs common for all sensors
 *
 */

#ifndef __SENSOR_H__
#define __SENSOR_H__

#include "hw/hw_interface.h"

typedef struct {
    uint8_t addr;              // Address of the sensor (7 lower bits for I2C / CS GPIO pin for SPI)
    HwInterfaceType_t if_type; // Hardware interface type (e.g. HW_INTERFACE_I2C or HW_INTERFACE_SPI)
} SensorInfo_t;

/**
 * @struct SensorError_t
 * @brief Error codes returned by sensor API functions
 */
typedef enum {
    SENSOR_ERR_OK = 0x00,            /**< Operation finished successfully */
    SENSOR_ERR_NULL_ARGUMENT,        /**< Error: NULL ptr passed as argument */
    SENSOR_ERR_HW_INTERFACE_FAILURE, /**< Error: Hardware interface operation failure / sensor is not responding */
    SENSOR_ERR_INVALID_ID,           /** < Error: Sensor invalid ID */
    SENSOR_ERR_NOT_INITIALIZED,      /**< Error: Sensor not initialized yet */
    SENSOR_ERR_GENERIC,              /**< Error: Generic error */
} SensorError_t;

#endif // __SENSOR_H__