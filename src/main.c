#include "app/app.h"
#include "utils/log.h"

int main() {
    AppError_t err = app_init();
    if(err != APP_ERR_OK) {
        log_error("app_init failed (ret: %d)", err);
        return -1;
    }
    log_info("App controller initialized");

    err = app_run();
    if(err != APP_ERR_OK) {
        log_error("app_run failed (ret: %d)", err);
        return -1;
    }
    log_info("App controller running...");

    // err = app_stop();
    // err = app_deinit();

    return 0;
}