/**
 * @file i2c_bus.h
 * @brief Wrapper API for master I2C communication over spidev interface
 *
 * @note Designed to provide thread-safe functionality (MT-Safe)
 *
 * @note Use i2c_bus_init(), and i2c_bus_deinit() to initialize and deinitialize new I2C bus instance.
 * Use: i2c_bus_read() and i2c_bus_write() to read and write single or multiple bytes to/from
 * specific registers in the slave device.
 */

#ifndef __I2C_BUS_H__
#define __I2C_BUS_H__

#include <stdint.h> // For: std types
#include <stdlib.h> // For: size_t

/**
 * @struct SensorError_t
 * @brief Error codes returned by sensor API functions
 */
typedef enum {
    I2C_BUS_ERR_OK = 0x00,       /**< Operation finished successfully */
    I2C_BUS_ERR_NULL_ARGUMENT,   /**< Error: NULL ptr passed as argument */
    I2C_BUS_ERR_I2CDEV_FAILURE,  /**< Error: Linux's spidev interface failure */
    I2C_BUS_ERR_PTHREAD_FAILURE, /**< Error: Pthread API call failure */
    I2C_BUS_ERR_GENERIC,         /**< Error: Generic error */
} I2CBusError_t;

typedef struct {
    int i2c_adapter; // I2C Adapter number (assigned dynamically by Linux kernel)
} I2CBusConfig_t;

typedef struct I2CBus_t {
    I2CBusConfig_t cfg;   // I2C Bus configuration
    pthread_mutex_t lock; // Lock for all read/write operations
    int fd;               // I2C interface file descriptor
} I2CBus_t;

/**
 * @brief Initialize a new I2C Bus instance.
 *
 * Opens the I2C adapter and sets up the slave address for communication.
 *
 * @param[in, out] ctx Pointer to the I2CBus_t instance.
 * @param[in] cfg Configuration structure.
 * @return I2C_BUS_ERR_OK on success, I2C_BUS_ERR_NULL_ARGUMENT or I2C_BUS_ERR_I2CDEV_FAILURE otherwise.
 */
I2CBusError_t i2c_bus_init(I2CBus_t* ctx, const I2CBusConfig_t cfg);

/**
 * @brief Perform a burst read from a specific register over I2C.
 *
 * Uses I2C_RDWR ioctl with two messages (write register address, read data).
 *
 * @param[in] ctx Pointer to the I2CBus_t instance.
 * @param[in] slave_addr Address of the slave device (7 lower bits)
 * @param[in] reg_addr Register address to read from.
 * @param[out] buf Buffer to store the read data.
 * @param[in] len Number of bytes to read.
 * @return I2C_BUS_ERR_OK on success, error code otherwise.
 */
I2CBusError_t i2c_bus_read(I2CBus_t* ctx, const uint8_t slave_addr, const uint8_t reg_addr, uint8_t* buf, const size_t len);

/**
 * @brief Write data to a specific register over I2C.
 *
 * Sends the register address and data in one write() call.
 *
 * @param[in] ctx Pointer to the I2CBus_t instance.
 * @param[in] slave_addr Address of the slave device (7 lower bits)
 * @param[in] reg_addr Register address to write to.
 * @param[in] data Pointer to the data to write.
 * @param[in] len Number of bytes to write.
 * @return I2C_BUS_ERR_OK on success, error code otherwise.
 */
I2CBusError_t
i2c_bus_write(I2CBus_t* ctx, const uint8_t slave_addr, const uint8_t reg_addr, const uint8_t* data, const size_t len);

/**
 * @brief Deinitialize the I2C Bus instance and release resources.
 *
 * Closes the file descriptor and clears the I2CBus_t structure.
 *
 * @param[in, out] ctx Pointer to the I2CBus_t instance.
 * @return I2C_BUS_ERR_OK on success, error code otherwise.
 */
I2CBusError_t i2c_bus_deinit(I2CBus_t* ctx);

#endif // __I2C_BUS_H__