#include "hw/i2c_bus.h"

#include <errno.h>         // For: errno
#include <fcntl.h>         // For: open() and related macros
#include <linux/i2c-dev.h> // For: I2C related macros
#include <linux/i2c.h>     // For: I2C related structs and macros
#include <stdio.h>         // For: snprintf()
#include <string.h>        // For: memset()
#include <sys/ioctl.h>     // For: ioctl() and related macros
#include <unistd.h>        // For: read(), write(), close()

#include "utils/log.h"


I2CBusError_t i2c_bus_init(I2CBus_t* ctx, const I2CBusConfig_t cfg) {
    if(!ctx) {
        return I2C_BUS_ERR_NULL_ARGUMENT;
    }

    // Zero-out the I2CBus_t struct on init
    memset(ctx, 0, sizeof(I2CBus_t));

    // Open the I2C adapter device file
    char filename[20];
    snprintf(filename, 19, "/dev/i2c-%d", cfg.i2c_adapter);
    int fd = open(filename, O_RDWR);
    if(fd < 0) {
        log_error("open() returned: %d (err: %s)", fd, strerror(errno));
        return I2C_BUS_ERR_I2CDEV_FAILURE;
    }

    // Set the address of the I2C slave device
    int ret = ioctl(fd, I2C_SLAVE, (long)cfg.slave_addr);
    if(ret < 0) {
        log_error("ioctl() returned: %d (err: %s)", ret, strerror(errno));
        close(fd); // Try closing the I2C adapter device file
        return I2C_BUS_ERR_I2CDEV_FAILURE;
    }

    // Populate data in the context struct (cfg, fd)
    ctx->fd = fd;
    ctx->cfg = cfg;

    return I2C_BUS_ERR_OK;
}

I2CBusError_t i2c_bus_read(const I2CBus_t* ctx, const uint8_t reg_addr, uint8_t* buf, const size_t len) {
    if(!ctx || !buf) {
        return I2C_BUS_ERR_NULL_ARGUMENT;
    }

    // Prepare data to be sent (ioctl used instead of write() to support burst read requests)
    struct i2c_msg msg[2]; // Do not change to dynamic allocation

    // First message: Address of the register from which data will be read
    uint8_t addr = reg_addr;
    struct i2c_msg specify_reg = { .addr = ctx->cfg.slave_addr, .buf = &addr, .flags = 0, .len = sizeof(reg_addr) };
    msg[0] = specify_reg;

    // Second message: Read 'len' bytes from the selected register
    struct i2c_msg read = { .addr = ctx->cfg.slave_addr, .buf = buf, .flags = I2C_M_RD, .len = len };
    msg[1] = read;

    // Pack messages
    struct i2c_rdwr_ioctl_data packet = { .msgs = msg, .nmsgs = (sizeof(msg) / sizeof(msg[0])) };

    // Perform combined write/read transaction
    if(ioctl(ctx->fd, I2C_RDWR, &packet) < 0) {
        log_error("failed to read data (dev:0x%02X, reg:0x%02X, err: %s)", ctx->cfg.slave_addr, reg_addr, strerror(errno));
        return I2C_BUS_ERR_I2CDEV_FAILURE;
    }

    log_debug("read %zu bytes (dev:0x%02X, reg:0x%02X)", len, ctx->cfg.slave_addr, reg_addr);
    return I2C_BUS_ERR_OK;
}

I2CBusError_t i2c_bus_write(const I2CBus_t* ctx, const uint8_t reg_addr, const uint8_t* data, const size_t len) {
    if(!ctx || !data) {
        return I2C_BUS_ERR_NULL_ARGUMENT;
    }

    // Perform an atomic write of register address and actual data
    uint8_t buf[len + 1];
    buf[0] = reg_addr;          // Specify register address (first byte sent over I2C)
    memcpy(&buf[1], data, len); // Copy the data to be sent (consecutive bytes)

    if(write(ctx->fd, buf, len + 1) != len + 1) {
        log_error("failed to send data (dev:0x%02X, reg:0x%02X, err: %s)", ctx->cfg.slave_addr, reg_addr, strerror(errno));
        return I2C_BUS_ERR_I2CDEV_FAILURE;
    }

    log_debug("wrote %zu bytes (dev:0x%02X, reg:0x%02X)", len, ctx->cfg.slave_addr, reg_addr);
    return I2C_BUS_ERR_OK;
}

I2CBusError_t i2c_bus_deinit(I2CBus_t* ctx) {
    if(!ctx) {
        return I2C_BUS_ERR_NULL_ARGUMENT;
    }

    // Close the I2C adapter device file
    int ret = close(ctx->fd);
    if(ret < 0) {
        log_error("close() returned: %d (err: %s)", ret, strerror(errno));
        return I2C_BUS_ERR_I2CDEV_FAILURE;
    }

    // Zero-out the I2CBus_t struct on deinit
    memset(ctx, 0, sizeof(I2CBus_t));

    return I2C_BUS_ERR_OK;
}