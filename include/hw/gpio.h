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

GpioError_t gpio_init(Gpio_t* ctx);
GpioError_t gpio_set(Gpio_t* ctx, const uint8_t line, const uint8_t state);
GpioError_t gpio_get(Gpio_t* ctx, const uint8_t line, uint8_t* state);
GpioError_t gpio_deinit(Gpio_t* ctx);

#endif // __GPIO_H__