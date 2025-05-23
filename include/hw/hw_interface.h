/**
 * @file hw_interface.h
 * @brief Wrapper API for all hardware interface operations (both I2C and SPI)
 *
 * @note Thread-safety should be provided by lower-level components (e.g. i2c_bus)
 *
 * @note Use hw_interface_init(), and hw_interface_deinit() to initialize and deinitialize new I2C/SPI bus
 * instance. Use: hw_interface_read() and hw_interface_write() to read and write single or multiple bytes
 * to/from specific registers in the slave device.
 */

#ifndef __HW_INTERFACE_H__
#define __HW_INTERFACE_H__

#include <stdint.h> // For: std types
#include <stdlib.h> // For: size_t

#include "hw/i2c_bus.h"
#include "hw/spi_bus.h"

/**
 * @struct HwInterfaceError_t
 * @brief Error codes returned by hw interface API functions
 */
typedef enum {
    HW_INTERFACE_ERR_OK = 0x00,            /**< Operation finished successfully */
    HW_INTERFACE_ERR_NULL_ARGUMENT,        /**< Error: NULL ptr passed as argument */
    HW_INTERFACE_ERR_INIT_FAILURE,         /**< Error: Interface init failure */
    HW_INTERFACE_ERR_DEINIT_FAILURE,       /**< Error: Interface deinit failure */
    HW_INTERFACE_ERR_TRANSMISSION_FAILURE, /**< Error: Linux's spidev/i2cdev interface failure */
    HW_INTERFACE_ERR_GENERIC,              /**< Error: Generic error */
} HwInterfaceError_t;

/**
 * @struct HwInterfaceType_t
 * @brief Type of the hardware interface to be used for communication
 */
typedef enum {
    HW_INTERFACE_I2C = 0x00, /**< Protocol: I2C */
    HW_INTERFACE_SPI,        /**< Protocol: SPI */
} HwInterfaceType_t;

/**
 * @struct HwInterface_t
 * @brief Hardware interface context - I2CBus_t or SPIBus_t depending on the type selected on initialization
 */
typedef struct {
    HwInterfaceType_t type; // Type of the interface (e.g. SPI/I2C)
    union {
        I2CBus_t i2c;
        SPIBus_t spi;
    } handle; // Handle for a specific i2c/spi instance
} HwInterface_t;

/**
 * @brief Initialize a new Hardware Interface instance.
 *
 * Open the I2C/SPI adapter and set it up for communication.
 *
 * @param[in, out] ctx Pointer to the HwInterface_t instance.
 * @param[in] type Hardware interface type (HW_INTERFACE_I2C or HW_INTERFACE_SPI)
 * @return HW_INTERFACE_ERR_OK on success, HW_INTERFACE_ERR_NULL_ARGUMENT or HW_INTERFACE_ERR_INIT_FAILURE otherwise.
 */
HwInterfaceError_t hw_interface_init(HwInterface_t* ctx, const HwInterfaceType_t type);

/**
 * @brief Perform a burst read from a selected register.
 *
 * @param[in, out] ctx Pointer to the HwInterface_t instance.
 * @param[in] slave_addr Address of the slave device (7 lower bits)
 * @param[in] reg_addr Register address to read from.
 * @param[out] buf Buffer to store the read data.
 * @param[in] len Number of bytes to read.
 * @return HW_INTERFACE_ERR_OK on success, error code otherwise.
 */
HwInterfaceError_t
hw_interface_read(HwInterface_t* ctx, const uint8_t slave_addr, const uint8_t reg_addr, uint8_t* buf, const size_t len);

/**
 * @brief Write data to a selected register.
 *
 * @param[in] ctx Pointer to the HwInterface_t instance.
 * @param[in] slave_addr Address of the slave device (7 lower bits for I2C / CS GPIO pin for SPI).
 * @param[in] reg_addr Register address to write to.
 * @param[in] data Pointer to the data to write.
 * @param[in] len Number of bytes to write.
 * @return HW_INTERFACE_ERR_OK on success, error code otherwise.
 */
HwInterfaceError_t
hw_interface_write(HwInterface_t* ctx, const uint8_t slave_addr, const uint8_t reg_addr, const uint8_t* data, const size_t len);

/**
 * @brief Deinitialize the hardware interface instance and release resources.
 *
 * @param[in, out] ctx Pointer to the HwInterface_t instance.
 * @return HW_INTERFACE_ERR_OK on success, HW_INTERFACE_ERR_NULL_ARGUMENT or HW_INTERFACE_ERR_DEINIT_FAILURE otherwise.
 */
HwInterfaceError_t hw_interface_deinit(HwInterface_t* ctx);

#endif // __HW_INTERFACE_H__
