#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
// Cmocka must be included last (!)
#include <cmocka.h>

#include "hw/hw_interface.h"
#include "sensors/bme280.h"

uint8_t bme280_mock_memory[256] = {
    // Default values
    [0x00 ... 0xFF] = 0x00,

    // Chip ID
    [0xD0] = 0x60, // Correct Chip ID for BME280

    // Temperature calibration
    [0x88] = 0x6B,
    [0x89] = 0x70, // dig_T1 = 0x706B = 28747
    [0x8A] = 0x67,
    [0x8B] = 0x33, // dig_T2 = 0x3367 = 13159
    [0x8C] = 0xFC,
    [0x8D] = 0x18, // dig_T3 = 0x18FC = 6396

    // Pressure calibration
    [0x8E] = 0x8E,
    [0x8F] = 0x6D, // dig_P1 = 0x6D8E = 28206
    [0x90] = 0xD0,
    [0x91] = 0xB3, // dig_P2 = 0xB3D0 = -4624
    [0x92] = 0x0B,
    [0x93] = 0xD0, // dig_P3 = 0xD00B = 5339
    [0x94] = 0x0B,
    [0x95] = 0x27, // dig_P4 = 0x270B = 10083
    [0x96] = 0x00,
    [0x97] = 0x8C, // dig_P5 = 0x8C00 = -228
    [0x98] = 0xF9,
    [0x99] = 0xFF, // dig_P6 = 0xFFF9 = -7
    [0x9A] = 0x7C,
    [0x9B] = 0x3C, // dig_P7 = 0x3C7C = 15500
    [0x9C] = 0x48,
    [0x9D] = 0xC5, // dig_P8 = 0xC548 = -14600
    [0x9E] = 0x70,
    [0x9F] = 0x17, // dig_P9 = 0x1770 = 6000

    // Humidity calibration
    [0xA1] = 75, // dig_H1
    [0xE1] = 0x6A,
    [0xE2] = 0x01, // dig_H2 = 0x016A = 362
    [0xE3] = 0x00, // dig_H3
    [0xE4] = 0x45, // dig_H4 (MSB = 0x45)
    [0xE5] = 0x03, // H4/H5 cross
    [0xE6] = 0x1E, // dig_H6 = 30

    // ctrl_hum = 0x01 (oversampling x1)
    [0xF2] = 0x01,

    // ctrl_meas = 0x27 (osrs_t, osrs_p = x1, mode = normal)
    [0xF4] = 0x27,

    // config = 0x00 (default)
    [0xF5] = 0x00,

    // Raw pressure = 0x2C6C00 = 2888192 (to match 1050 hPa)
    [0xF7] = 0x2C,
    [0xF8] = 0x6C,
    [0xF9] = 0x00,

    // Raw temperature = 0x189600 = 1636800 (to match 25°C)
    [0xFA] = 0x18,
    [0xFB] = 0x96,
    [0xFC] = 0x00,

    // Raw humidity = 0x5200 = 21120 (to match 55% RH)
    [0xFD] = 0x52,
    [0xFE] = 0x00,
};

/************************ Mock and stub functions ************************/

HwInterfaceError_t __wrap_hw_interface_init(HwInterface_t* ctx, const HwInterfaceType_t type) {
    return HW_INTERFACE_ERR_OK;
}

HwInterfaceError_t
__wrap_hw_interface_read(HwInterface_t* ctx, const uint8_t slave_addr, const uint8_t reg_addr, uint8_t* buf, const size_t len) {
    for(int i = 0; i < len; i++) {
        *(buf + i) = *(bme280_mock_memory + reg_addr);
    }

    return HW_INTERFACE_ERR_OK;
}

HwInterfaceError_t
__wrap_hw_interface_write(HwInterface_t* ctx, const uint8_t slave_addr, const uint8_t reg_addr, const uint8_t* data, const size_t len) {
    return HW_INTERFACE_ERR_OK;
}

HwInterfaceError_t __wrap_hw_interface_deinit(HwInterface_t* ctx) {
    return HW_INTERFACE_ERR_OK;
}

/********************* Auxiliary functions *********************/

/************************ Unit tests ************************/

static void test_bme280_init_success(void** state) {
    (void)state;
    Bme280_t ctx;
    HwInterface_t hw_ctx;
    assert_int_equal(bme280_init(&ctx, 0x00, hw_ctx), SENSOR_ERR_OK);
}

static void test_bme280_init_null_context(void** state) {
    (void)state;
    HwInterface_t hw_ctx;
    assert_int_equal(bme280_init(NULL, 0x00, hw_ctx), SENSOR_ERR_NULL_ARGUMENT);
}

static void test_bme280_get_temp_null_context(void** state) {
    (void)state;
    float temp;
    assert_int_equal(bme280_get_temp(NULL, &temp), SENSOR_ERR_NULL_ARGUMENT);
}

static void test_bme280_get_temp_null_output(void** state) {
    (void)state;
    Bme280_t ctx = { .is_initialized = true };
    assert_int_equal(bme280_get_temp(&ctx, NULL), SENSOR_ERR_NULL_ARGUMENT);
}

static void test_bme280_get_temp_uninitialized_context(void** state) {
    (void)state;
    Bme280_t ctx = { .is_initialized = false };
    float temp;
    assert_int_equal(bme280_get_temp(&ctx, &temp), SENSOR_ERR_NOT_INITIALIZED);
}

static void test_bme280_get_temp_valid_read(void** state) {
    (void)state;
    Bme280_t ctx;
    HwInterface_t hw_ctx;
    assert_int_equal(bme280_init(&ctx, 0x00, hw_ctx), SENSOR_ERR_OK);

    float temp = 0;
    assert_int_equal(bme280_get_temp(&ctx, &temp), SENSOR_ERR_OK);
    assert_in_range(temp, 24.0, 26.0);
}

static void test_bme280_get_hum_valid_read(void** state) {
    (void)state;
    Bme280_t ctx;
    HwInterface_t hw_ctx;
    bme280_init(&ctx, 0x00, hw_ctx);

    float hum = 0;
    assert_int_equal(bme280_get_hum(&ctx, &hum), SENSOR_ERR_OK);
    assert_in_range(hum, 54.0, 56.0);
}

static void test_bme280_get_press_valid_read(void** state) {
    (void)state;
    Bme280_t ctx;
    HwInterface_t hw_ctx;
    bme280_init(&ctx, 0x00, hw_ctx);

    float press = 0;
    assert_int_equal(bme280_get_press(&ctx, &press), SENSOR_ERR_OK);
    assert_in_range(press, 104000, 106000);
}

/*********************** Runner ************************/

int run_bme280_tests(void) {
    const struct CMUnitTest bme280_tests[] = {
        cmocka_unit_test(test_bme280_init_success),
        cmocka_unit_test(test_bme280_init_null_context),
        cmocka_unit_test(test_bme280_get_temp_null_context),
        cmocka_unit_test(test_bme280_get_temp_null_output),
        cmocka_unit_test(test_bme280_get_temp_uninitialized_context),
        cmocka_unit_test(test_bme280_get_temp_valid_read),
        cmocka_unit_test(test_bme280_get_hum_valid_read),
        cmocka_unit_test(test_bme280_get_press_valid_read),
    };

    return cmocka_run_group_tests(bme280_tests, NULL, NULL);
}