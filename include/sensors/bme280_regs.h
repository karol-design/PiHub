/**
 * @file bme280_regs.h
 * @brief Register map and helper unions/structs for working with BME280
 */

#ifndef __BME280_REGS_H__
#define __BME280_REGS_H__

#include <stdint.h> // For: standard types

/**
 * Map of BME280 registers
 */
#define BME280_REG_HUM_LSB 0xFE // Data regs (hum, temp, press): read only
#define BME280_REG_HUM_MSB 0xFD
#define BME280_REG_TEMP_XLSB 0xFC
#define BME280_REG_TEMP_MSB 0xFB
#define BME280_REG_TEMP_LSB 0xFA
#define BME280_REG_PRESS_XLSB 0xF9
#define BME280_REG_PRESS_LSB 0xF8
#define BME280_REG_PRESS_MSB 0xF7
#define BME280_REG_CONFIG 0xF5       // Control reg: partial read/write
#define BME280_REG_CTRL_MEAS 0xF4    // Control reg: read/write
#define BME280_REG_STATUS 0xF3       // Status reg: partial read only
#define BME280_REG_CTRL_HUM 0xF2     // Control reg: partial read/write
#define BME280_REG_CALIB_B_LENGTH 16 // Number of registers used for calibration data (section B)
#define BME280_REG_CALIB_B_BASE 0xE1 // Base address of all registers with calibration data (section B)
#define BME280_REG_RESET 0xE0        // Reset reg: write only
#define BME280_REG_ID 0xD0           // Chip ID: read only
#define BME280_REG_CALIB_A_LENGTH 26 // Number of registers used for calibration data (section A)
#define BME280_REG_CALIB_A_BASE 0x88 // Base address of all registers with calibration data (section A)


// Settings
#define BME280_FILTER_OFF 0            // b000 for turning off the filter
#define BME280_SPI3W_ENABLED 1         // b1 for enabling 3-wire SPI interface
#define BME280_SPI3W_DISABLED 0        // b0 for disabling 3-wire SPI interface
#define BME280_STANDBY_MAX_TIME 7      // b111 for 20 ms standby time
#define BME280_OSRS_MAX_OVERSAMPLING 5 // b101 for oversampling x16
#define BME280_NORMAL_MODE 3           // b11 for normal mode

// Structure definitions for registers divided into multiple regions via bitfields

/**
 * @union ConfigReg_t
 * @brief Sets the rate, filter and interface options of the device
 * @note Writes to the “config” register in normal mode may be ignored. In sleep mode writes are not ignored.
 */
typedef union {
    struct {
        uint8_t t_sb : 3;
        uint8_t filter : 3;
        uint8_t reserved : 1;
        uint8_t spi3w_en : 1;
    } b;
    uint8_t w;
} ConfigReg_t;

/**
 * @union CtrlMeasReg_t
 * @brief Sets the press and temp data acquisition options (oversampling and mode)
 * @note Needs to be written after changing "ctrl_hum" for the changes to become effective.
 */
typedef union {
    struct {
        uint8_t osrs_t : 3;
        uint8_t osrs_p : 3;
        uint8_t mode : 2;
    } b;
    uint8_t w;
} CtrlMeasReg_t;

/**
 * @union CtrlHumReg_t
 * @brief Sets the hum data acquisition options (oversampling)
 * @note Changes to this register only become effective after a write operation to “ctrl_meas”.
 */
typedef union {
    struct {
        uint8_t reserved : 5;
        uint8_t osrs_h : 3;
    } b;
    uint8_t w;
} CtrlHumReg_t;

/**
 * @union StatusReg_t
 * @brief Holds the status of the device (measuring status, NVM data access)
 */
typedef union {
    struct {
        uint8_t reserved_1 : 4;
        uint8_t measuring : 1;
        uint8_t reserved_2 : 2;
        uint8_t im_update : 1;
    } b;
    uint8_t w;
} StatusReg_t;

#endif // __BME280_REGS_H__