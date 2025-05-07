#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
// Cmocka must be included last (!)
#include <cmocka.h>

#include "hw/hw_interface.h"
#include "sensors/bme280.h"

uint8_t bme280_mock_memory[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x8f, 0x50, 0x89, 0x62, 0x71, 0x60, 0xa8, 0x06, 0x91, 0x6e, 0x1c, 0x67, 0x32, 0x00, 0xae, 0x89, 0x55,
    0xd6, 0xd0, 0x0b, 0xc8, 0x1e, 0x05, 0x00, 0xf9, 0xff, 0xac, 0x26, 0x0a, 0xd8, 0xbd, 0x10,

    0x00, 0x4b, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00, 0xc0, 0x00,
    0x54, 0x00, 0x00, 0x00, 0x00, 0x60, 0x02, 0x00, 0x01, 0xff, 0xff, 0x1f, 0x4e, 0x08, 0x00,

    0x00, 0x40, 0xb7, 0xff, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x77, 0x01, 0x00, 0x12, 0x21, 0x03, 0x1e, 0x50, 0x41, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x05, 0x04, 0xb7, 0xe0, 0x00, 0x54, 0x73, 0xb0, 0x7d, 0xf7, 0xe0, 0x5e, 0x99, 0x80 };

/************************ Mock and stub functions ************************/

HwInterfaceError_t __wrap_hw_interface_init(HwInterface_t* ctx, const HwInterfaceType_t type) {
    return HW_INTERFACE_ERR_OK;
}

HwInterfaceError_t
__wrap_hw_interface_read(HwInterface_t* ctx, const uint8_t slave_addr, const uint8_t reg_addr, uint8_t* buf, const size_t len) {
    for(int i = 0; i < len; i++) {
        *(buf + i) = bme280_mock_memory[reg_addr + i];
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
    assert_in_range(temp, 19.0, 20.0); // The temperature when the memory snapshot has been taken was 19.88 *C
}

static void test_bme280_get_hum_valid_read(void** state) {
    (void)state;
    Bme280_t ctx;
    HwInterface_t hw_ctx;
    bme280_init(&ctx, 0x00, hw_ctx);

    float hum = 0;
    assert_int_equal(bme280_get_hum(&ctx, &hum), SENSOR_ERR_OK);
    assert_in_range(hum, 31.0, 33.0); // The rel humidity when the memory snapshot has been taken was around 33.0%
}

static void test_bme280_get_press_valid_read(void** state) {
    (void)state;
    Bme280_t ctx;
    HwInterface_t hw_ctx;
    bme280_init(&ctx, 0x00, hw_ctx);

    float press = 0;
    assert_int_equal(bme280_get_press(&ctx, &press), SENSOR_ERR_OK);
    assert_in_range(press, 101300, 101400); // The pressure when the memory snapshot has been taken was 1013.3667 hPa
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