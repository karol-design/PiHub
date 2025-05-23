/**
 * @file bme280.h
 * @brief A simple driver for Bosh BME280 digital humidity, pressure and temperature sensor with I2C and SPI support
 * @TODO: Add mutex for sensors (two users using the same sensor at the same time)
 *
 */

#ifndef __BME280_H__
#define __BME280_H__

#include <stdbool.h> // For: boolean type

#include "hw/hw_interface.h"
#include "sensors/sensor.h"

/**
 * @struct Trim_t
 * @brief Trimming parameters (programmed into the devices' non-volatile memory during production)
 */
typedef struct {
    // Temperature compensation related values
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    // Pressure compensation related values
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    // Humidity compensation related values
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
} Trim_t;

/**
 * @struct Bme280_t
 * @brief Include sensor address, hardware interface handle, calibration data and init flag.
 */
typedef struct {
    uint8_t addr;         // Address of the sensor (7 lower bits for I2C / CS GPIO pin for SPI)
    HwInterface_t hw_ctx; // Hardware interface context
    bool is_initialized;  // Initialization flag
    Trim_t calib;         // Calibration digits
} Bme280_t;

/**
 * @brief Initialize a new BME280 instance
 * @param[in, out]  ctx  Pointer to the Bme280_t instance
 * @param[in]  addr  Address of the sensor (7 lower bits for I2C / CS GPIO pin for SPI)
 * @param[in] hw_ctx Handle of the hardware interface to be used by the sensor (I2C/SPI)
 * @return SENSOR_ERR_OK on success, SENSOR_ERR_NULL_ARG or SENSOR_ERR_PTHREAD_FAILURE otherwise
 */
SensorError_t bme280_init(Bme280_t* ctx, const uint8_t addr, HwInterface_t hw_ctx);

/**
 * @brief Read the current temperature in degrees Celsius.
 *
 * @param[in] ctx Pointer to an initialized Bme280_t instance.
 * @param[out] temp Pointer to a float where the Celsius temperature (e.g. 21.50 *C) will be saved (resolution: 0.01 *C).
 * @return SENSOR_ERR_OK on success, SENSOR_ERR_NULL_ARG / SENSOR_ERR_NOT_INITIALIZED / SENSOR_ERR_HW_INTERFACE_FAILURE otherwise.
 */
SensorError_t bme280_get_temp(Bme280_t* ctx, float* temp);

/**
 * @brief Read the current relative humidity (as a percentage).
 *
 * @param[in] ctx Pointer to an initialized Bme280_t instance.
 * @param[out] temp Pointer to a float where the relative humidity (e.g. 86.058 %) will be saved.
 * @return SENSOR_ERR_OK on success, SENSOR_ERR_NULL_ARG / SENSOR_ERR_NOT_INITIALIZED / SENSOR_ERR_HW_INTERFACE_FAILURE otherwise.
 */
SensorError_t bme280_get_hum(Bme280_t* ctx, float* hum);

/**
 * @brief Read the current pressure in Pascals
 *
 * @param[in] ctx Pointer to an initialized Bme280_t instance.
 * @param[out] temp Pointer to a float where the pressure in Pascals (e.g. 96386.2 Pa) will be saved.
 * @return SENSOR_ERR_OK on success, SENSOR_ERR_NULL_ARG / SENSOR_ERR_NOT_INITIALIZED / SENSOR_ERR_HW_INTERFACE_FAILURE otherwise.
 */
SensorError_t bme280_get_press(Bme280_t* ctx, float* press);

/**
 * @brief Check BME280 sensor ID
 *
 * @param[in] ctx Pointer to an initialized Bme280_t instance.
 * @return SENSOR_ERR_OK if the sensor is responsive and returns correct ID, SENSOR_ERR_NULL_ARGUMENT /
 * SENSOR_ERR_HW_INTERFACE_FAILURE / SENSOR_ERR_INVALID_ID otherwise.
 */
SensorError_t bme280_check_id(Bme280_t* ctx);

/**
 * @brief Deinit Bme280_t sensor instance (zero out the structure, including the init flag)
 *
 * @param[in] ctx Pointer to an initialized Bme280_t instance.
 * @return SENSOR_ERR_OK on success, SENSOR_ERR_NULL_ARGUMENT or SENSOR_ERR_NOT_INITIALIZED otherwise.
 */
SensorError_t bme280_deinit(Bme280_t* ctx);

#endif // __BME280_H__