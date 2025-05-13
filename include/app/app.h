/**
 * @file app.h
 * @brief A core PiHub app controller responsible for initializing and managing other components like
 * network manager, command dispatcher, system stats, hardware, and sensor modules
 *
 * @note This component IS NOT thread-safe and is designed so that there is ONLY ONE APP CONTROLLER INSTANCE
 * managed by the main thread.
 *
 * @note Use app_init(), and app_deinit() to initialize and deinitialize new App controller instance.
 * Use: app_run() to run the server and the core controller logic.
 */

#ifndef __APP_H__
#define __APP_H__

#include "app/dispatcher.h"
#include "comm/network.h"
#include "hw/gpio.h"
#include "hw/hw_interface.h"
#include "sensors/bme280.h"

typedef enum {
    APP_ERR_OK = 0x00,            /**< Operation finished successfully */
    APP_ERR_NULL_ARG,             /**< Error: NULL pointer passed as argument */
    APP_ERR_INVALID_ARG,          /**< Error: Incorrect parameter passed */
    APP_ERR_SERVER_FAILURE,       /**< Error: Server (network) failure */
    APP_ERR_DISPATCHER_FAILURE,   /**< Error: Command dispatcher failure */
    APP_ERR_HW_INTERFACE_FAILURE, /**< Error: Hardware interface (I2C/SPI) failure */
    APP_ERR_SENSOR_FAILURE,       /**< Error: Sensor failure */
    APP_ERR_GPIO_FAILURE,         /**< Error: GPIO failure */
    APP_ERR_NOT_STARTED,          /**< Error: The app controller has not been started yet */
    APP_ERR_RUNNING,              /**< Error: The app controller is running */
    APP_ERR_GENERIC               /**< Error: Generic error */
} AppError_t;

AppError_t app_init(void);
AppError_t app_run(void);
AppError_t app_stop(void);
AppError_t app_deinit(void);

#endif // __APP_H__