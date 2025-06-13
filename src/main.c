#include <signal.h>            // for: sig_atomic_t
#include <systemd/sd-daemon.h> // for: systemd notifications

#include "app/app.h"
#include "utils/config.h"
#include "utils/log.h"

volatile sig_atomic_t sig_status = 0;

static void catch_function(int signo) {
    sig_status = signo;
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0); // Disable stdout buffering completely

    // Set the signal handler for SIGINT
    if(signal(SIGINT, catch_function) == SIG_ERR) {
        log_error("failed to set the SIGINT signal handler");
        return EXIT_FAILURE;
    }

    log_info("Version: %d.%d (compiled: %s %s)", VER_MAJOR, VER_MINOR, __DATE__, __TIME__);

    // Initialize app controller
    AppError_t err = app_init();
    if(err != APP_ERR_OK) {
        log_error("app_init failed (ret: %d)", err);
        return EXIT_FAILURE;
    }
    log_info("App controller initialized");

    // Run the application
    err = app_run();
    if(err != APP_ERR_OK) {
        log_error("app_run failed (ret: %d)", err);
        return EXIT_FAILURE;
    }
    log_info("App controller running...");

    // Notify systemd that the service is ready
    sd_notify(0, "READY=1");

    while(1) {
        // Enter an infinite empty loop once the app controller and the server is started
        sleep(1);

        // Regularly check for SIGINT and SIGTERM signals to shut down the daemon and exit cleanly
        if(sig_status == SIGINT || sig_status == SIGTERM) {
            sd_notify(0, "STOPPING=1"); // Notify systemd that the service will be stopped

            err = app_stop(); // Stop the app controller
            if(err != APP_ERR_OK) {
                log_error("app_stop failed (err: %d)", err);
                return EXIT_FAILURE;
            }

            err = app_deinit(); // Deinit the controller
            if(err != APP_ERR_OK) {
                log_error("app_deinit failed (err: %d)", err);
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS; // Exit
        }
    }

    // This should never be reached
    return EXIT_SUCCESS;
}