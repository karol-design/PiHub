#include "sensors/bme280.h"

#include <string.h> // For: memset

#include "hw/i2c_bus.h"
#include "sensors/bme280_regs.h"
#include "sensors/sensor.h"
#include "utils/common.h"
#include "utils/config.h"
#include "utils/log.h"

#define BME280_ID 0x60 // Device ID common for all BME280 sensors

typedef int32_t BME280_S32_t;
typedef uint32_t BME280_U32_t;
typedef int64_t BME280_S64_t;

SensorError_t bme280_get_reading(struct Sensor* ctx, const SensorMeasurement_t type, SensorOutput_t* out);
STATIC SensorError_t bme280_get_temp(Sensor_t* ctx, float* temp);
STATIC SensorError_t bme280_get_hum(Sensor_t* ctx, float* hum);
STATIC SensorError_t bme280_get_press(Sensor_t* ctx, float* press);
SensorError_t bme280_get_status(Sensor_t* ctx);
SensorError_t bme280_deinit(Sensor_t* ctx);

SensorError_t bme280_init(Sensor_t* ctx, const uint8_t addr, HwInterface_t hw_ctx) {
    if(!ctx) {
        return SENSOR_ERR_NULL_ARGUMENT;
    }

    // Zero-out the Sensor_t struct on init and populate addr and hw_ctx
    memset(ctx, 0, sizeof(Sensor_t));
    ctx->addr = addr;
    ctx->hw_ctx = hw_ctx;

    SensorError_t s_ret = bme280_get_status(ctx);
    if(s_ret != SENSOR_ERR_OK) {
        return s_ret;
    }

    // Assign function pointers
    ctx->get_reading = bme280_get_reading;
    ctx->get_status = bme280_get_status;
    ctx->deinit = bme280_deinit;

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

    // Set: max standby (20ms); filter off; 3-wire SPI off
    ConfigReg_t config_reg = { .b.filter = BME280_FILTER_OFF, .b.t_sb = BME280_STANDBY_MAX_TIME, .b.spi3w_en = BME280_SPI3W_DISABLED };
    h_ret = hw_interface_write(&hw_ctx, addr, BME280_REG_CONFIG, &config_reg.w, sizeof(config_reg.w));
    if(h_ret != HW_INTERFACE_ERR_OK) {
        log_error("failed to write to the Config reg (err: %d)", h_ret);
        return SENSOR_ERR_HW_INTERFACE_FAILURE;
    }

    ctx->is_initialized = true;

    return SENSOR_ERR_OK;
}

SensorError_t bme280_get_reading(struct Sensor* ctx, const SensorMeasurement_t type, SensorOutput_t* out) {
    if(!ctx) {
        return SENSOR_ERR_NULL_ARGUMENT;
    }
    SensorError_t err;

    // Zero-out the SensorOutput_t union before saving the output
    memset(out, 0, sizeof(SensorOutput_t));

    switch(type) {
    case SENSOR_MEASUREMENT_TEMP: {
        err = bme280_get_temp(ctx, &out->temp);
        break;
    }
    case SENSOR_MEASUREMENT_HUM: {
        err = bme280_get_hum(ctx, &out->hum);
        break;
    }
    case SENSOR_MEASUREMENT_PRESS: {
        err = bme280_get_press(ctx, &out->press);
        break;
    }
    default: {
        err = SENSOR_ERR_OP_NOT_SUPPORTED;
    }
    }

    return err;
}

STATIC SensorError_t bme280_get_temp(Sensor_t* ctx, float* temp) {
    /* Data readout is done by starting a burst read from 0xF7 to 0xFC (temperature and pressure) or
     * from 0xF7 to 0xFE (temperature, pressure and humidity). The data are read out in an unsigned
     * 20-bit format both for pressure and for temperature and in an unsigned 16-bit format for
     * humidity.*/
    uint8_t temp_buf[3];
    hw_interface_read(&ctx->hw_ctx, ctx->addr, BME280_REG_TEMP_LSB, temp_buf, sizeof(temp_buf));
    log_info("temperature: %hu | %hu | %hu", temp_buf[0], temp_buf[1], temp_buf[2]);
    *temp = (temp_buf[0] + temp_buf[1] + temp_buf[2]);

    return SENSOR_ERR_OK;
}

STATIC SensorError_t bme280_get_hum(Sensor_t* ctx, float* hum) {
    return SENSOR_ERR_OK;
}

STATIC SensorError_t bme280_get_press(Sensor_t* ctx, float* press) {
    return SENSOR_ERR_OK;
}

SensorError_t bme280_get_status(Sensor_t* ctx) {
    // Check sensor ID to confirm the communication link is working and the sensor is on (should be 0x60 for BM280)
    uint8_t id_buf;

    HwInterfaceError_t err = hw_interface_read(&ctx->hw_ctx, ctx->addr, BME280_REG_ID, &id_buf, sizeof(id_buf));
    if(err != HW_INTERFACE_ERR_OK) {
        log_error("hw_interface_read returned %d", err);
        return SENSOR_ERR_HW_INTERFACE_FAILURE;
    } else if(id_buf != BME280_ID) {
        log_error("sensor returned id: %02X instead of %02X", id_buf, BME280_ID);
        return SENSOR_ERR_NOT_RESPONDING;
    }

    return SENSOR_ERR_OK;
}

SensorError_t bme280_deinit(Sensor_t* ctx) {
    if(!ctx) {
        return SENSOR_ERR_NULL_ARGUMENT;
    } else if(!ctx->is_initialized) {
        return SENSOR_ERR_NOT_INITIALIZED;
    }

    // Zero-out the Sensor_t struct on deinit
    memset(ctx, 0, sizeof(Sensor_t));

    return SENSOR_ERR_OK;
}

// // Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// // t_fine carries fine temperature as global value
// BME280_S32_t t_fine;
// BME280_S32_t BME280_compensate_T_int32(BME280_S32_t adc_T) {
//     BME280_S32_t var1, var2, T;
//     var1 = ((((adc_T >> 3) - ((BME280_S32_t)dig_T1 << 1))) * ((BME280_S32_t)dig_T2)) >> 11;
//     var2 = (((((adc_T >> 4) - ((BME280_S32_t)dig_T1)) * ((adc_T >> 4) - ((BME280_S32_t)dig_T1))) >> 12) *
//            ((BME280_S32_t)dig_T3)) >>
//     14;
//     t_fine = var1 + var2;
//     T = (t_fine * 5 + 128) >> 8;
//     return T;
// }