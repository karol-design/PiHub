#include "hw/hw_interface.h"

#include "utils/log.h"

#define I2C_ADAPTER 1 // On RPI the I2C adapter is mounted as '/dev/i2c-1'

HwInterfaceError_t hw_interface_init(HwInterface_t* ctx, const HwInterfaceType_t type) {
    if(!ctx) {
        return HW_INTERFACE_ERR_NULL_ARGUMENT;
    }
    ctx->type = type; // Save the type of the interface to be used in the context

    switch(type) {
    case HW_INTERFACE_I2C: {
        I2CBusConfig_t cfg = { .i2c_adapter = I2C_ADAPTER };
        I2CBusError_t err = i2c_bus_init(&ctx->handle.i2c, cfg);
        if(err != I2C_BUS_ERR_OK) {
            log_error("failed to initialize the i2c adapter (err: %d)", err);
            return HW_INTERFACE_ERR_INIT_FAILURE;
        }
        break;
    }
    case HW_INTERFACE_SPI: {
        break;
    }
    }

    return HW_INTERFACE_ERR_OK;
}

HwInterfaceError_t
hw_interface_read(HwInterface_t* ctx, const uint8_t slave_addr, const uint8_t reg_addr, uint8_t* buf, const size_t len) {
    if(!ctx || !buf) {
        return HW_INTERFACE_ERR_NULL_ARGUMENT;
    }

    switch(ctx->type) {
    case HW_INTERFACE_I2C: {
        I2CBusError_t err = i2c_bus_read(&ctx->handle.i2c, slave_addr, reg_addr, buf, len);
        if(err != I2C_BUS_ERR_OK) {
            log_error("failed to receive data over the i2c adapter (err: %d)", err);
            return HW_INTERFACE_ERR_TRANSMISSION_FAILURE;
        }
        break;
    }
    case HW_INTERFACE_SPI: {
        break;
    }
    }

    return HW_INTERFACE_ERR_OK;
}

HwInterfaceError_t
hw_interface_write(HwInterface_t* ctx, const uint8_t slave_addr, const uint8_t reg_addr, const uint8_t* data, const size_t len) {
    if(!ctx || !data) {
        return HW_INTERFACE_ERR_NULL_ARGUMENT;
    }

    switch(ctx->type) {
    case HW_INTERFACE_I2C: {
        I2CBusError_t err = i2c_bus_write(&ctx->handle.i2c, slave_addr, reg_addr, data, len);
        if(err != I2C_BUS_ERR_OK) {
            log_error("failed to send data over the i2c adapter (err: %d)", err);
            return HW_INTERFACE_ERR_TRANSMISSION_FAILURE;
        }
        break;
    }
    case HW_INTERFACE_SPI: {
        break;
    }
    }

    return HW_INTERFACE_ERR_OK;
}

HwInterfaceError_t hw_interface_deinit(HwInterface_t* ctx) {
    if(!ctx) {
        return HW_INTERFACE_ERR_NULL_ARGUMENT;
    }

    switch(ctx->type) {
    case HW_INTERFACE_I2C: {
        I2CBusError_t err = i2c_bus_deinit(&ctx->handle.i2c);
        if(err != I2C_BUS_ERR_OK) {
            log_error("failed to deinitialize the i2c adapter (err: %d)", err);
            return HW_INTERFACE_ERR_DEINIT_FAILURE;
        }
        break;
    }
    case HW_INTERFACE_SPI: {
        break;
    }
    }

    return HW_INTERFACE_ERR_OK;
}