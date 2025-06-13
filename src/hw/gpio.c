#include "hw/gpio.h"

#include <errno.h>  // For: errno
#include <string.h> // For: memset

#include "utils/log.h"

#define GPIO_CHIP_PATH "/dev/gpiochip0"
#define GPIO_CONSUMER "PiHub"

GpioError_t gpio_init(Gpio_t* ctx) {
    if(!ctx) {
        return GPIO_ERR_NULL_ARGUMENT;
    }

    // Zero out context on init
    memset(ctx, 0, sizeof(Gpio_t));

    // Initialize mutex for protecting gpio-related critical sections
    int err = pthread_mutex_init(&ctx->lock, NULL);
    if(err != 0) {
        log_error("pthread_mutex_init() returned %d", err);
        return GPIO_ERR_PTHREAD_FAILURE;
    }

    ctx->chip = gpiod_chip_open(GPIO_CHIP_PATH);
    if(!ctx->chip) {
        log_error("Failed to open a GPIO chip, gpiod_chip_open_by_name returned NULL");
        return GPIO_ERR_INIT_FAILURE;
    }

    return GPIO_ERR_OK;
}

GpioError_t gpio_set(Gpio_t* ctx, const uint8_t line_num, const uint8_t value) {
    if(!ctx) {
        return GPIO_ERR_NULL_ARGUMENT;
    } else if(!ctx->chip) {
        return GPIO_ERR_NOT_INITIALIZED;
    }

    // Critical section; Mutex required
    int ret_p = pthread_mutex_lock(&ctx->lock);
    if(ret_p != 0) {
        log_error("pthread_mutex_lock() returned %d", ret_p);
        return GPIO_ERR_PTHREAD_FAILURE;
    }
    log_debug("gpio lock taken");

    GpioError_t err = GPIO_ERR_OK;
    struct gpiod_line* line;
    do {
        line = gpiod_chip_get_line(ctx->chip, line_num);
        if(!line) {
            log_error("Get line failed (errno: %d)", errno);
            err = GPIO_ERR_LIBGPIOD_FAILURE;
            break;
        }

        int ret = gpiod_line_request_output(line, GPIO_CONSUMER, value);
        if(ret < 0) {
            log_error("Request line as output failed (errno: %d)", errno);
            err = GPIO_ERR_LIBGPIOD_FAILURE;
            break;
        }

        ret = gpiod_line_set_value(line, value);
        if(ret < 0) {
            log_error("Set line output failed (errno: %d)", errno);
            err = GPIO_ERR_LIBGPIOD_FAILURE;
            break;
        }
    } while(0); // Run only once (do not loop)

    gpiod_line_release(line);

    ret_p = pthread_mutex_unlock(&ctx->lock);
    if(ret_p != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret_p);
        err = GPIO_ERR_PTHREAD_FAILURE;
    }
    log_debug("gpio lock released");

    return err;
}

GpioError_t gpio_get(Gpio_t* ctx, const uint8_t line_num, uint8_t* state) {
    if(!ctx || !state) {
        return GPIO_ERR_NULL_ARGUMENT;
    } else if(!ctx->chip) {
        return GPIO_ERR_NOT_INITIALIZED;
    }

    // Critical section; Mutex required
    int ret_p = pthread_mutex_lock(&ctx->lock);
    if(ret_p != 0) {
        log_error("pthread_mutex_lock() returned %d", ret_p);
        return GPIO_ERR_PTHREAD_FAILURE;
    }
    log_debug("gpio lock taken");

    GpioError_t err = GPIO_ERR_OK;
    struct gpiod_line* line;
    do {
        line = gpiod_chip_get_line(ctx->chip, line_num);
        if(!line) {
            log_error("Get line failed (errno: %d)", errno);
            err = GPIO_ERR_LIBGPIOD_FAILURE;
            break;
        }

        int ret = gpiod_line_request_input(line, GPIO_CONSUMER);
        if(ret < 0) {
            log_error("Request line as input failed (errno: %d)", errno);
            err = GPIO_ERR_LIBGPIOD_FAILURE;
            break;
        }

        ret = gpiod_line_get_value(line);
        if(ret < 0) {
            log_error("Get line input failed (errno: %d)", errno);
            err = GPIO_ERR_LIBGPIOD_FAILURE;
            break;
        }
        *state = ret; // gpiod_line_get_value returns -1 on error; GPIO line value (0 or 1) otherwise
    } while(0); // Run only once (do not loop)

    gpiod_line_release(line);

    ret_p = pthread_mutex_unlock(&ctx->lock);
    if(ret_p != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret_p);
        return GPIO_ERR_PTHREAD_FAILURE;
    }
    log_debug("gpio lock released");

    return err;
}

GpioError_t gpio_deinit(Gpio_t* ctx) {
    if(!ctx) {
        return GPIO_ERR_NULL_ARGUMENT;
    } else if(!ctx->chip) {
        return GPIO_ERR_NOT_INITIALIZED;
    }

    gpiod_chip_close(ctx->chip);

    int ret_p = pthread_mutex_destroy(&ctx->lock);
    if(ret_p != 0) {
        log_error("pthread_mutex_destroy() returned %d", ret_p);
        return GPIO_ERR_PTHREAD_FAILURE;
    }

    // Do not leave a "dangling" pointer
    ctx->chip = NULL;

    return GPIO_ERR_OK;
}