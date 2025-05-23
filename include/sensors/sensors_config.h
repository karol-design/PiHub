/**
 * @file sensors_config.h
 * @brief Configuration file for sensors used on the board
 *
 */

#ifndef __SENSORS_CONFIG_H__
#define __SENSORS_CONFIG_H__

#include "sensors/sensor.h"

#define BME280_COUNT 1

static const SensorInfo_t SENSORS_CONFIG_BME280[BME280_COUNT] = {
    { .addr = 0x76, .if_type = HW_INTERFACE_I2C }
    // Add more bme280 sensors here
};

#endif // __SENSORS_CONFIG_H__