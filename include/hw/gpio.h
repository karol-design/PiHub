/**
 * @file gpio.h
 * @brief A simple wrapper for libgpiod GPIO control API
 *
 * @note Designed to provide thread-safe functionality (MT-Safe)
 *
 */

#ifndef __GPIO_H__
#define __GPIO_H__

#include <gpiod.h>   // For: GPIO-related API
#include <pthread.h> // For: mutex-relate API
#include <stdint.h>  // For: standard int types

/**
 * @struct GpioError_t
 * @brief Error codes returned by GPIO API functions
 */
typedef enum {
    GPIO_ERR_OK = 0x00,        /**< Operation finished successfully */
    GPIO_ERR_NULL_ARGUMENT,    /**< Error: NULL ptr passed as argument */
    GPIO_ERR_NOT_INITIALIZED,  /**< Error: GPIO Chip not initialized yet */
    GPIO_ERR_LIBGPIOD_FAILURE, /**< Error: libgpiod API operation failure */
    GPIO_ERR_INIT_FAILURE,     /**< Error: Failed to open the GPIO chip */
    GPIO_ERR_PTHREAD_FAILURE,  /**< Error: Pthread API failure (e.g. mutex lock/unlock) */
    GPIO_ERR_GENERIC,          /**< Error: Generic error */
} GpioError_t;

/**
 * @struct Gpio_t
 * @brief Include a ptr to the GPIO chip and mutex for protecting critical sections.
 */
typedef struct {
    struct gpiod_chip* chip;
    pthread_mutex_t lock;
} Gpio_t;

/**
 * @brief Initialize the GPIO context and open the GPIO chip.
 *
 * Initializes the internal mutex and opens a handle to the default GPIO chip (e.g., /dev/gpiochip0).
 * Must be called before any other GPIO operations.
 *
 * @param[in, out] ctx Pointer to the Gpio_t structure to be initialized.
 * @return GPIO_ERR_OK on success, appropriate error code otherwise (e.g. GPIO_ERR_NULL_ARGUMENT, GPIO_ERR_PTHREAD_FAILURE, GPIO_ERR_INIT_FAILURE).
 */
GpioError_t gpio_init(Gpio_t* ctx);

/**
 * @brief Set the value of a specified GPIO line.
 *
 * Requests control of the line, sets it as output, and writes the desired value.
 *
 * @param[in, out] ctx Pointer to the initialized Gpio_t context.
 * @param[in] line GPIO line number to set.
 * @param[in] state Desired output value (0 = low, 1 = high).
 * @return GPIO_ERR_OK on success, appropriate error code otherwise (e.g. GPIO_ERR_NULL_ARGUMENT, GPIO_ERR_NOT_INITIALIZED, GPIO_ERR_LIBGPIOD_FAILURE).
 */
GpioError_t gpio_set(Gpio_t* ctx, const uint8_t line, const uint8_t state);

/**
 * @brief Get the current value of a specified GPIO line.
 *
 * Requests control of the line, sets it as input, and reads its current value.
 *
 * @param[in, out] ctx Pointer to the initialized Gpio_t context.
 * @param[in] line GPIO line number to read.
 * @param[out] state Pointer to store the read value (0 or 1).
 * @return GPIO_ERR_OK on success, appropriate error code otherwise (e.g. GPIO_ERR_NULL_ARGUMENT, GPIO_ERR_NOT_INITIALIZED, GPIO_ERR_LIBGPIOD_FAILURE).
 */
GpioError_t gpio_get(Gpio_t* ctx, const uint8_t line, uint8_t* state);

/**
 * @brief Deinitialize the GPIO context and release resources.
 *
 * Closes the GPIO chip and destroys the associated mutex.
 *
 * @param[in, out] ctx Pointer to the Gpio_t structure to deinitialize.
 * @return GPIO_ERR_OK on success, appropriate error code otherwise (e.g. GPIO_ERR_NULL_ARGUMENT, GPIO_ERR_NOT_INITIALIZED, GPIO_ERR_PTHREAD_FAILURE).
 */
GpioError_t gpio_deinit(Gpio_t* ctx);

#endif // __GPIO_H__