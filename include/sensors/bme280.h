/**
 * @file bme280.h
 * @brief A driver for Bosh BME280 digital humidity, pressure and temperature sensor with I2C and SPI support
 *
 * @note Use the universal sensor interface defined in sensor.h
 */

#ifndef __BME280_H__
#define __BME280_H__

#include "sensors/sensor.h"

/**
 * @brief Initialize a new BME280 instance
 * @param[in, out]  ctx  Pointer to the Sensor instance
 * @param[in]  addr  Address of the sensor (7 lower bits for I2C / CS GPIO pin for SPI)
 * @param[in] hw_ctx Handle of the hardware interface to be used by the sensor (I2C/SPI)
 * @return SENSOR_ERR_OK on success, SENSOR_ERR_NULL_ARG or SENSOR_ERR_PTHREAD_FAILURE otherwise
 */
SensorError_t bme280_init(Sensor_t* ctx, const uint8_t addr, HwInterface_t hw_ctx);

#endif // __BME280_H__