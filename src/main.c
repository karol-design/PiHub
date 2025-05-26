#include "app/app.h"
#include "utils/log.h"

int main() {
    // Initialize app controller
    AppError_t err = app_init();
    if(err != APP_ERR_OK) {
        log_error("app_init failed (ret: %d)", err);
        return -1;
    }
    log_info("App controller initialized");

    // Run the application
    err = app_run();
    if(err != APP_ERR_OK) {
        log_error("app_run failed (ret: %d)", err);
        return -1;
    }
    log_info("App controller running...");

    // err = app_stop();
    // err = app_deinit();

    // Exit the main thread once the app is set up and running
    return 0;
}