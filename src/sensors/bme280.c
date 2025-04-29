#include "sensors/bme280.h"

#include <string.h> // For: memset
#include <unistd.h> // For: usleep

#include "sensors/bme280_regs.h"
#include "utils/common.h"
#include "utils/config.h"
#include "utils/log.h"

#define BME280_ID 0x60                // Device ID common for all BME280 sensors
#define BME280_TEMP_SCALE 100.0f      // Temperature scale from x100 *C to *C
#define BME280_PRESS_SCALE 256.0f     // Pressure scale from Q24.8 to float
#define BME280_HUM_SCALE 1024.0f      // Humidity scale from Q22.10 format to percents
#define BME280_SOFT_RESET_DELAY_MS 10 // Obligatory delay after a soft reset is performed

typedef int32_t Bme280_s32_t;
typedef uint32_t Bme280_u32_t;
typedef int64_t Bme280_s64_t;

/**
 * @struct Bme280_temp_t
 * @brief Temperature output which includes temperature in degrees Celsius (x100) as well as fine temp value (for pressure and hum calculations)
 */
typedef struct {
    Bme280_s32_t deg_C; // Temperature in x100 *C [resolution: 0.01 DegC, output value of “5123” equals 51.23 *C]
    Bme280_s32_t fine; // Fine temperature value for press and hum compensation calc
} Bme280_temp_t;

/**
 * @struct Bme280_output_t
 * @brief Include compensated and converted temperature, pressure and humidity
 */
typedef struct {
    Bme280_s32_t t; // Temperature in deg. C with 0.01 DegC resolution (e.g. 5123/100 = 51.23 DegC.)
    Bme280_u32_t p; // Pressure in Q24.8 format (e.g. 24674867/256 = 96386.2 Pa = 963.862 hPa)
    Bme280_u32_t h; // Humidity in Q22.10 format (e.g. 47445/1024 = 46.333 %RH)
} Bme280_output_t;

/**
 * @brief Read and compensate measurement data from the BME280 sensor.
 *
 * @param[in] ctx Pointer to initialized BME280 sensor instance.
 * @param[out] out Pointer to structure to hold compensated output data.
 * @return SENSOR_ERR_OK on success, SENSOR_ERR_NULL_ARGUMENT / SENSOR_ERR_NOT_INITIALIZED / SENSOR_ERR_HW_INTERFACE_FAILURE otherwise
 */
STATIC SensorError_t bme280_data_readout(Bme280_t* ctx, Bme280_output_t* out);

/**
 * @brief Read and compensate measurement data from the BME280 sensor.
 *
 * @param[in] ctx Pointer to initialized BME280 sensor instance.
 * @return SENSOR_ERR_OK on success, SENSOR_ERR_NULL_ARGUMENT / SENSOR_ERR_NOT_INITIALIZED / SENSOR_ERR_HW_INTERFACE_FAILURE otherwise
 */
STATIC SensorError_t bme280_read_trim_params(Bme280_t* ctx);

/**
 * @brief Compensate temperature measurement from the BME280 sensor.
 *
 * @param[in] trim Pointer to calibration digits
 * @param[in] adc_T Raw temperature data retrieved from sensor's registers
 * @return Bme280_temp_t struct with temperature in DegC, resolution is 0.01 DegC an fine temp for hum calculation. Output value of “5123” equals 51.23 DegC.
 */
STATIC Bme280_temp_t BME280_compensate_T_int32(Trim_t trim, Bme280_s32_t adc_T);

/**
 * @brief Compensate pressure measurement from the BME280 sensor.
 *
 * @param[in] trim Pointer to calibration digits
 * @param[in] adc_P Raw pressure data retrieved from sensor's registers
 * @param[in] t_fine Current fine temperature (required to calculate the pressure)
 * @return Pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
 */
STATIC Bme280_u32_t BME280_compensate_P_int64(Trim_t trim, Bme280_s32_t adc_P, Bme280_s32_t t_fine);

/**
 * @brief Compensate humidity measurement from the BME280 sensor.
 *
 * @param[in] trim Pointer to calibration digits
 * @param[in] adc_H Raw humidity data retrieved from sensor's registers
 * @param[in] t_fine Current fine temperature (required to calculate the humidity)
 * @return Humidity in %RH as unsigned 32-bit integer in Q22.10 format (22 integer and 10 fractional bits; e.g. “47445” represents 47445 / 1024 = 46.333 %RH).
 */
STATIC Bme280_u32_t bme280_compensate_H_int32(Trim_t trim, Bme280_s32_t adc_H, Bme280_s32_t t_fine);

SensorError_t bme280_init(Bme280_t* ctx, const uint8_t addr, HwInterface_t hw_ctx) {
    if(!ctx) {
        return SENSOR_ERR_NULL_ARGUMENT;
    }

    // Zero-out the Bme280_t struct on init and populate addr and hw_ctx
    memset(ctx, 0, sizeof(Bme280_t));
    ctx->addr = addr;
    ctx->hw_ctx = hw_ctx;

    SensorError_t s_ret = bme280_check_id(ctx);
    if(s_ret != SENSOR_ERR_OK) {
        return s_ret;
    }

    // Set: max oversampling (x16) on temp and press measurements; Normal mode
    CtrlMeasReg_t ctrl_meas_reg = { .b.osrs_p = BME280_OSRS_MAX_OVERSAMPLING,
        .b.osrs_t = BME280_OSRS_MAX_OVERSAMPLING,
        .b.mode = BME280_NORMAL_MODE };
    HwInterfaceError_t h_ret =
    hw_interface_write(&hw_ctx, addr, BME280_REG_CTRL_MEAS, &ctrl_meas_reg.w, sizeof(ctrl_meas_reg.w));
    if(h_ret != HW_INTERFACE_ERR_OK) {
        log_error("failed to write to the CtrlMeas reg (err: %d)", h_ret);
        return SENSOR_ERR_HW_INTERFACE_FAILURE;
    }

    // Wait for at least 10 ms after a soft reset (as recommended in the BME280 datasheet)
    usleep((useconds_t)BME280_SOFT_RESET_DELAY_MS * 1000);

    // Set: max standby (20ms); filter off; 3-wire SPI off
    ConfigReg_t config_reg = { .b.filter = BME280_FILTER_OFF, .b.t_sb = BME280_STANDBY_MAX_TIME, .b.spi3w_en = BME280_SPI3W_DISABLED };
    h_ret = hw_interface_write(&hw_ctx, addr, BME280_REG_CONFIG, &config_reg.w, sizeof(config_reg.w));
    if(h_ret != HW_INTERFACE_ERR_OK) {
        log_error("failed to write to the Config reg (err: %d)", h_ret);
        return SENSOR_ERR_HW_INTERFACE_FAILURE;
    }

    // Read trim (calibration) parameters
    s_ret = bme280_read_trim_params(ctx);
    if(s_ret != SENSOR_ERR_OK) {
        return s_ret;
    }

    ctx->is_initialized = true;

    return SENSOR_ERR_OK;
}

SensorError_t bme280_get_temp(Bme280_t* ctx, float* temp) {
    if(!ctx || !temp) {
        return SENSOR_ERR_NULL_ARGUMENT;
    } else if(!ctx->is_initialized) {
        return SENSOR_ERR_NOT_INITIALIZED;
    }

    Bme280_output_t out;
    SensorError_t err = bme280_data_readout(ctx, &out);
    if(err != SENSOR_ERR_OK) {
        return err;
    }

    // Scale the temperature to degrees Celsius
    *temp = (float)out.t / BME280_TEMP_SCALE;

    return SENSOR_ERR_OK;
}

SensorError_t bme280_get_hum(Bme280_t* ctx, float* hum) {
    if(!ctx || !hum) {
        return SENSOR_ERR_NULL_ARGUMENT;
    } else if(!ctx->is_initialized) {
        return SENSOR_ERR_NOT_INITIALIZED;
    }

    Bme280_output_t out;
    SensorError_t err = bme280_data_readout(ctx, &out);
    if(err != SENSOR_ERR_OK) {
        return err;
    }

    // Convert the humidity to percents
    *hum = (float)out.h / BME280_HUM_SCALE;

    return SENSOR_ERR_OK;
}

SensorError_t bme280_get_press(Bme280_t* ctx, float* press) {
    if(!ctx || !press) {
        return SENSOR_ERR_NULL_ARGUMENT;
    } else if(!ctx->is_initialized) {
        return SENSOR_ERR_NOT_INITIALIZED;
    }

    Bme280_output_t out;
    SensorError_t err = bme280_data_readout(ctx, &out);
    if(err != SENSOR_ERR_OK) {
        return err;
    }

    // Convert the pressure to Pascals (can be further converted to hPa by dividing by 100.0)
    *press = (float)out.p / BME280_PRESS_SCALE;

    return SENSOR_ERR_OK;
}

SensorError_t bme280_check_id(Bme280_t* ctx) {
    if(!ctx) {
        return SENSOR_ERR_NULL_ARGUMENT;
    }

    // Check sensor ID to confirm the communication link is working and the sensor is on (should be 0x60 for BM280)
    uint8_t id_buf;

    HwInterfaceError_t err = hw_interface_read(&ctx->hw_ctx, ctx->addr, BME280_REG_ID, &id_buf, sizeof(id_buf));
    if(err != HW_INTERFACE_ERR_OK) {
        log_error("hw_interface_read returned %d", err);
        return SENSOR_ERR_HW_INTERFACE_FAILURE;
    } else if(id_buf != BME280_ID) {
        log_error("sensor returned id: %02X instead of %02X", id_buf, BME280_ID);
        return SENSOR_ERR_INVALID_ID;
    }

    return SENSOR_ERR_OK;
}

SensorError_t bme280_deinit(Bme280_t* ctx) {
    if(!ctx) {
        return SENSOR_ERR_NULL_ARGUMENT;
    } else if(!ctx->is_initialized) {
        return SENSOR_ERR_NOT_INITIALIZED;
    }

    // Zero-out the Bme280_t struct on deinit
    memset(ctx, 0, sizeof(Bme280_t));

    return SENSOR_ERR_OK;
}

STATIC SensorError_t bme280_read_trim_params(Bme280_t* ctx) {
    if(!ctx) {
        return SENSOR_ERR_NULL_ARGUMENT;
    }

    uint8_t calib_buf[BME280_REG_CALIB_A_LENGTH + BME280_REG_CALIB_B_LENGTH];
    uint8_t* buf_ptr = calib_buf;
    HwInterfaceError_t err;

    // Read both parts of the calibration data from the sensor's registers
    err = hw_interface_read(&ctx->hw_ctx, ctx->addr, BME280_REG_CALIB_A_BASE, buf_ptr, BME280_REG_CALIB_A_LENGTH);
    if(err != HW_INTERFACE_ERR_OK) {
        log_error("failed to read Calibration Data (part A), hw_interface_read returned: %d", err);
        return SENSOR_ERR_HW_INTERFACE_FAILURE;
    }

    buf_ptr = calib_buf + BME280_REG_CALIB_A_LENGTH;
    err = hw_interface_read(&ctx->hw_ctx, ctx->addr, BME280_REG_CALIB_B_BASE, buf_ptr, BME280_REG_CALIB_B_LENGTH);
    if(err != HW_INTERFACE_ERR_OK) {
        log_error("failed to read Calibration Data (part B), hw_interface_read returned: %d", err);
        return SENSOR_ERR_HW_INTERFACE_FAILURE;
    }

    // Cast the retrieved data onto actual calibration digits
    Trim_t* calib = &ctx->calib;
    uint8_t* d = calib_buf;

    // Temperature compensation (0x88 - 0x8D)
    calib->dig_T1 = (uint16_t)(d[1] << 8 | d[0]); // 0x88 and 0x89 ([7:0]/[15:8])
    calib->dig_T2 = (int16_t)(d[3] << 8 | d[2]);  // 0x8A and 0x8B
    calib->dig_T3 = (int16_t)(d[5] << 8 | d[4]);  // 0x8C and 0x8D

    // Pressure compensation (0x8E - 0x9F)
    calib->dig_P1 = (uint16_t)(d[7] << 8 | d[6]);  // 0x8E and 0x8F
    calib->dig_P2 = (int16_t)(d[9] << 8 | d[8]);   // 0x90 and 0x91
    calib->dig_P3 = (int16_t)(d[11] << 8 | d[10]); // 0x92 and 0x93
    calib->dig_P4 = (int16_t)(d[13] << 8 | d[12]); // 0x94 and 0x95
    calib->dig_P5 = (int16_t)(d[15] << 8 | d[14]); // 0x96 and 0x97
    calib->dig_P6 = (int16_t)(d[17] << 8 | d[16]); // 0x98 and 0x99
    calib->dig_P7 = (int16_t)(d[19] << 8 | d[18]); // 0x9A and 0x9B
    calib->dig_P8 = (int16_t)(d[21] << 8 | d[20]); // 0x9C and 0x9D
    calib->dig_P9 = (int16_t)(d[23] << 8 | d[22]); // 0x9E and 0x9F

    // Humidity compensation
    calib->dig_H1 = d[25];                         // 0xA1
    calib->dig_H2 = (int16_t)(d[27] << 8 | d[26]); // 0xE1 and 0xE2
    calib->dig_H3 = d[28];                         // 0xE3
    // H4 is split across 0xE4 and 0xE5[3:0]
    calib->dig_H4 = (int16_t)((d[29] << 4) | (d[30] & 0x0F)); // 0xE4 << 4 | (0xE5 & 0x0F)
    // H5 is split across 0xE5[7:4] and 0xE6
    calib->dig_H5 = (int16_t)((d[31] << 4) | (d[30] >> 4)); // 0xE6 << 4 | (0xE5 >> 4)
    calib->dig_H6 = (int8_t)d[32];                          // 0xE7

    return SENSOR_ERR_OK;
}

STATIC SensorError_t bme280_data_readout(Bme280_t* ctx, Bme280_output_t* out) {
    if(!ctx || !out) {
        return SENSOR_ERR_NULL_ARGUMENT;
    } else if(!ctx->is_initialized) {
        return SENSOR_ERR_NOT_INITIALIZED;
    }

    uint8_t buf[BME280_REG_DATA_LENGTH];

    HwInterfaceError_t err_hw = hw_interface_read(&ctx->hw_ctx, ctx->addr, BME280_REG_PRESS_MSB, buf, sizeof(buf));
    if(err_hw != HW_INTERFACE_ERR_OK) {
        log_error("Failed to perform measurement data redout, hw_interface_read() returned: %d", err_hw);
        return SENSOR_ERR_HW_INTERFACE_FAILURE;
    }

    // Pressure: 20-bit unsigned (MSB, LSB, XLSB[7:4])
    uint32_t adc_P = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)(buf[2] & 0xF0) >> 4);
    // Temperature: 20-bit signed (MSB, LSB, XLSB[7:4])
    uint32_t adc_T = ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | ((uint32_t)(buf[5] & 0xF0) >> 4);
    // Humidity: 16-bit unsigned (optional, if enabled)
    uint32_t adc_H = ((uint32_t)buf[6] << 8) | buf[7];

    Bme280_temp_t t = BME280_compensate_T_int32(ctx->calib, (Bme280_s32_t)adc_T);
    out->t = t.deg_C;
    out->p = BME280_compensate_P_int64(ctx->calib, (Bme280_s32_t)adc_P, t.fine);
    out->h = bme280_compensate_H_int32(ctx->calib, (Bme280_s32_t)adc_H, t.fine);

    return SENSOR_ERR_OK;
}

STATIC Bme280_temp_t BME280_compensate_T_int32(Trim_t trim, Bme280_s32_t adc_T) {
    Bme280_temp_t out;
    Bme280_s32_t var1, var2;

    var1 = ((((adc_T >> 3) - ((Bme280_s32_t)trim.dig_T1 << 1))) * ((Bme280_s32_t)trim.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((Bme280_s32_t)trim.dig_T1)) * ((adc_T >> 4) - ((Bme280_s32_t)trim.dig_T1))) >> 12) *
            ((Bme280_s32_t)trim.dig_T3));
    var2 = var2 >> 14;

    out.fine = var1 + var2;
    out.deg_C = (out.fine * 5 + 128) >> 8;

    return out;
}

STATIC Bme280_u32_t BME280_compensate_P_int64(Trim_t trim, Bme280_s32_t adc_P, Bme280_s32_t t_fine) {
    Bme280_s64_t var1, var2, p;

    var1 = ((Bme280_s64_t)t_fine) - 128000;
    var2 = var1 * var1 * (Bme280_s64_t)trim.dig_P6;
    var2 = var2 + ((var1 * (Bme280_s64_t)trim.dig_P5) << 17);
    var2 = var2 + (((Bme280_s64_t)trim.dig_P4) << 35);
    var1 = ((var1 * var1 * (Bme280_s64_t)trim.dig_P3) >> 8) + ((var1 * (Bme280_s64_t)trim.dig_P2) << 12);
    var1 = (((((Bme280_s64_t)1) << 47) + var1)) * ((Bme280_s64_t)trim.dig_P1) >> 33;
    if(var1 == 0) {
        return 0; // avoid exception caused by division by zero
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((Bme280_s64_t)trim.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((Bme280_s64_t)trim.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((Bme280_s64_t)trim.dig_P7) << 4);

    return (Bme280_u32_t)p;
}

STATIC Bme280_u32_t bme280_compensate_H_int32(Trim_t trim, Bme280_s32_t adc_H, Bme280_s32_t t_fine) {
    Bme280_s32_t v_x1_u32r;

    v_x1_u32r = t_fine - ((Bme280_s32_t)76800);

    Bme280_s32_t tmp1 =
    (((adc_H << 14) - (((Bme280_s32_t)trim.dig_H4) << 20) - (((Bme280_s32_t)trim.dig_H5) * v_x1_u32r)) +
     ((Bme280_s32_t)16384)) >>
    15;

    Bme280_s32_t tmp2 = (((((v_x1_u32r * ((Bme280_s32_t)trim.dig_H6)) >> 10) *
                           (((v_x1_u32r * ((Bme280_s32_t)trim.dig_H3)) >> 11) + ((Bme280_s32_t)32768))) >>
                          10) +
                         ((Bme280_s32_t)2097152)) *
                        ((Bme280_s32_t)trim.dig_H2) +
                        8192;

    v_x1_u32r = (tmp1 * tmp2) >> 14;

    v_x1_u32r -= (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((Bme280_s32_t)trim.dig_H1)) >> 4);

    if(v_x1_u32r < 0)
        v_x1_u32r = 0;
    else if(v_x1_u32r > 419430400)
        v_x1_u32r = 419430400;

    return (Bme280_u32_t)(v_x1_u32r >> 12);
}