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
#define BME280_REG_DATA_LENGTH 8 // Number of registers used for temp/hum/press data
#define BME280_REG_HUM_LSB 0xFE  // Data regs (hum, temp, press): read only
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
#define BME280_REG_CALIB_B_LENGTH 7  // Number of registers used for calibration data (section B)
#define BME280_REG_CALIB_B_BASE 0xE1 // Base address of all registers with calibration data (section B)
#define BME280_REG_RESET 0xE0        // Reset reg: write only
#define BME280_REG_ID 0xD0           // Chip ID: read only
#define BME280_REG_CALIB_A_LENGTH 26 // Number of registers used for calibration data (section A)
#define BME280_REG_CALIB_A_BASE 0x88 // Base address of all registers with calibration data (section A)

// Calibration data
#define BME280_REG_DIG_T1 0x88 // and 0x89
#define BME280_REG_DIG_T2 0x8A // and 0x8B
#define BME280_REG_DIG_T3 0x8C // and 0x8C
#define BME280_REG_DIG_P1 0x8E // and 0x8F
#define BME280_REG_DIG_P2 0x90 // and 0x91
#define BME280_REG_DIG_P3 0x92 // and 0x93
#define BME280_REG_DIG_P4 0x94 // and 0x95
#define BME280_REG_DIG_P5 0x96 // and 0x97
#define BME280_REG_DIG_P6 0x98 // and 0x99
#define BME280_REG_DIG_P7 0x9A // and 0x9B
#define BME280_REG_DIG_P8 0x9C // and 0x9D
#define BME280_REG_DIG_P9 0x9E // and 0x9F
#define BME280_REG_DIG_H1 0xA1
#define BME280_REG_DIG_H2 0xE1 // and 0xE2
#define BME280_REG_DIG_H3 0xE3
#define BME280_REG_DIG_H4 0xE4 // and 0xE5 [3:0] (!)
#define BME280_REG_DIG_H5 0xE5 // only [7:4] and 0xE6 (!)


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
    struct __attribute__((packed)) {
        uint8_t spi3w_en : 1; // Bit 0: Enables 3-wire SPI interface when set to '1'
        uint8_t reserved : 1; // Bit 1: Reserved
        uint8_t filter : 3;   // Bits 2,3,4: Controls the time constant of the IIR filter
        uint8_t t_sb : 3;     // Bits 5,6,7: Controls inactive duration
    } b;
    uint8_t w;
} ConfigReg_t;

/**
 * @union CtrlMeasReg_t
 * @brief Sets the press and temp data acquisition options (oversampling and mode)
 * @note Needs to be written after changing "ctrl_hum" for the changes to become effective.
 */
typedef union {
    struct __attribute__((packed)) {
        uint8_t mode : 2;   // Bits 0,1: Control shte sensor mode of the device
        uint8_t osrs_p : 3; // Bits 2,3,4: Controls oversampling of pressure data
        uint8_t osrs_t : 3; // Bits: 5,6,7: Controls oversampling of temperature data
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
        uint8_t osrs_h : 3;   // Bits 0,1,2: Controls oversampling of humidity data
        uint8_t reserved : 5; // Bits 3,4,5,6,7: Reserved
    } b;
    uint8_t w;
} CtrlHumReg_t;

/**
 * @union StatusReg_t
 * @brief Holds the status of the device (measuring status, NVM data access)
 */
typedef union {
    struct {
        uint8_t im_update : 1; // Bit 0: Automatically set to '1' when NVM data are being copied to image registers
        uint8_t reserved_2 : 2; // Bit 1,2: Reserved
        uint8_t measuring : 1;  // Bit 3: Automatically set to '1' when a conversion is running
        uint8_t reserved_1 : 4; // Bit 4,5,6,7: Reserved
    } b;
    uint8_t w;
} StatusReg_t;

#endif // __BME280_REGS_H__