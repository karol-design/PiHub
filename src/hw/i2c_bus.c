#include "hw/i2c_bus.h"

#include <errno.h>         // For: errno
#include <fcntl.h>         // For: open() and related macros
#include <linux/i2c-dev.h> // For: I2C related macros
#include <linux/i2c.h>     // For: I2C related structs and macros
#include <pthread.h>       // For: pthread_mutex_t
#include <stdio.h>         // For: snprintf()
#include <string.h>        // For: memset()
#include <sys/ioctl.h>     // For: ioctl() and related macros
#include <unistd.h>        // For: read(), write(), close()

#include "utils/log.h"

#define I2C_DEV_MAX_PATH_LENGTH 20 // Maximum length of the I2C device file path (e.g. "/dev/i2c-1")

I2CBusError_t i2c_bus_init(I2CBus_t* ctx, const I2CBusConfig_t cfg) {
    if(!ctx) {
        return I2C_BUS_ERR_NULL_ARGUMENT;
    }

    // Zero-out the I2CBus_t struct on init
    memset(ctx, 0, sizeof(I2CBus_t));

    // Open the I2C adapter device file
    char filename[I2C_DEV_MAX_PATH_LENGTH + 1];
    snprintf(filename, I2C_DEV_MAX_PATH_LENGTH, "/dev/i2c-%d", cfg.i2c_adapter);
    int fd = open(filename, O_RDWR);
    if(fd < 0) {
        log_error("open() returned: %d (err: %s)", fd, strerror(errno));
        return I2C_BUS_ERR_I2CDEV_FAILURE;
    }

    // Initialize mutex for protecting access to the i2c
    int err = pthread_mutex_init(&ctx->lock, NULL);
    if(err != 0) {
        log_error("pthread_mutex_init() returned %d", err);
        return I2C_BUS_ERR_PTHREAD_FAILURE;
    }

    // Populate data in the context struct (cfg, fd)
    ctx->fd = fd;
    ctx->cfg = cfg;

    return I2C_BUS_ERR_OK;
}

I2CBusError_t i2c_bus_read(I2CBus_t* ctx, const uint8_t slave_addr, const uint8_t reg_addr, uint8_t* buf, const size_t len) {
    if(!ctx || !buf) {
        return I2C_BUS_ERR_NULL_ARGUMENT;
    }

    I2CBusError_t err = I2C_BUS_ERR_OK;

    // Accessing shared peripheral (critical section)
    int ret_p = pthread_mutex_lock(&ctx->lock);
    if(ret_p != 0) {
        log_error("pthread_mutex_lock() returned %d", ret_p);
        return I2C_BUS_ERR_PTHREAD_FAILURE;
    }
    log_debug("I2C lock taken");

    // Set the address of the I2C slave device
    int ret_i = ioctl(ctx->fd, I2C_SLAVE, slave_addr);
    if(ret_i < 0) {
        log_error("ioctl() returned: %d (err: %s)", ret_i, strerror(errno));
        err = I2C_BUS_ERR_I2CDEV_FAILURE;
    } else {
        // Prepare data to be sent (ioctl used instead of write() to support burst read requests)
        struct i2c_msg msg[2]; // Do not change to dynamic allocation

        // First message: Address of the register from which data will be read
        uint8_t addr = reg_addr;
        struct i2c_msg specify_reg = { .addr = slave_addr, .buf = &addr, .flags = 0, .len = sizeof(reg_addr) };
        msg[0] = specify_reg;

        // Second message: Read 'len' bytes from the selected register
        struct i2c_msg read = { .addr = slave_addr, .buf = buf, .flags = I2C_M_RD, .len = len };
        msg[1] = read;

        // Pack messages
        struct i2c_rdwr_ioctl_data packet = { .msgs = msg, .nmsgs = (sizeof(msg) / sizeof(msg[0])) };

        // Perform combined write/read transaction
        if(ioctl(ctx->fd, I2C_RDWR, &packet) < 0) {
            log_error("failed to read data (dev:0x%02X, reg:0x%02X, err: %s)", slave_addr, reg_addr, strerror(errno));
            err = I2C_BUS_ERR_I2CDEV_FAILURE;
        } else {
            log_debug("read %zu bytes (dev:0x%02X, reg:0x%02X)", len, slave_addr, reg_addr);
        }
    }

    ret_p = pthread_mutex_unlock(&ctx->lock);
    if(ret_p != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret_p);
        return I2C_BUS_ERR_PTHREAD_FAILURE;
    }
    log_debug("I2C lock released");

    return err;
}

I2CBusError_t
i2c_bus_write(I2CBus_t* ctx, const uint8_t slave_addr, const uint8_t reg_addr, const uint8_t* data, const size_t data_len) {
    if(!ctx || !data) {
        return I2C_BUS_ERR_NULL_ARGUMENT;
    }

    I2CBusError_t err = I2C_BUS_ERR_OK;

    // Accessing shared peripheral (critical section)
    int ret_p = pthread_mutex_lock(&ctx->lock);
    if(ret_p != 0) {
        log_error("pthread_mutex_lock() returned %d", ret_p);
        return I2C_BUS_ERR_PTHREAD_FAILURE;
    }
    log_debug("I2C lock taken");

    // Set the address of the I2C slave device
    int ret_i = ioctl(ctx->fd, I2C_SLAVE, slave_addr);
    if(ret_i < 0) {
        log_error("ioctl() returned: %d (err: %s)", ret_i, strerror(errno));
        err = I2C_BUS_ERR_I2CDEV_FAILURE;
    } else {
        // Perform an atomic write of register address and actual data
        uint8_t buf[data_len + 1];       // @TODO: no dynamically allocated mem (improve); VLA gcc feature
        buf[0] = reg_addr;               // Specify register address (first byte sent over I2C)
        memcpy(&buf[1], data, data_len); // Copy the data to be sent (consecutive bytes)

        if(write(ctx->fd, buf, data_len + 1) != data_len + 1) {
            log_error("failed to send data (dev:0x%02X, reg:0x%02X, err: %s)", slave_addr, reg_addr, strerror(errno));
            err = I2C_BUS_ERR_I2CDEV_FAILURE;
        } else {
            log_debug("wrote %zu bytes (dev:0x%02X, reg:0x%02X)", data_len, slave_addr, reg_addr);
        }
    }

    ret_p = pthread_mutex_unlock(&ctx->lock);
    if(ret_p != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret_p);
        return I2C_BUS_ERR_PTHREAD_FAILURE;
    }
    log_debug("I2C lock released");

    return err;
}

I2CBusError_t i2c_bus_deinit(I2CBus_t* ctx) {
    if(!ctx) {
        return I2C_BUS_ERR_NULL_ARGUMENT;
    }

    I2CBusError_t err = I2C_BUS_ERR_OK;

    // Deinit the I2C Bus (critical section)
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return I2C_BUS_ERR_PTHREAD_FAILURE;
    }
    log_debug("I2C lock taken");

    // Close the I2C adapter device file
    ret = close(ctx->fd);
    if(ret < 0) {
        log_error("close() returned: %d (err: %s)", ret, strerror(errno));
        err = I2C_BUS_ERR_I2CDEV_FAILURE;
    }

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return I2C_BUS_ERR_PTHREAD_FAILURE;
    }
    log_debug("I2C lock released");

    // Zero-out the I2CBus_t struct on deinit
    memset(ctx, 0, sizeof(I2CBus_t));

    return err;
}