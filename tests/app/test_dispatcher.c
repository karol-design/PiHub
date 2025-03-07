#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// Cmocka must be included last (!)
#include <cmocka.h>

#include "app/dispatcher.h"

#define FUNC_TEST_CMD_COUNT 5 // number of commands to be tested in a functional test

extern DispatcherError_t dispatcher_init(Dispatcher_t* ctx, const DispatcherConfig_t cfg);
extern DispatcherError_t dispatcher_register(Dispatcher_t* ctx, const uint32_t id, const DispatcherCommandDef_t cmd);
extern DispatcherError_t dispatcher_deregister(Dispatcher_t* ctx, const uint32_t id);
extern DispatcherError_t dispatcher_execute(Dispatcher_t* ctx, const char* buf);
extern DispatcherError_t dispatcher_deinit(Dispatcher_t* ctx);

/********************* Auxiliary functions *********************/

/**
 * @brief A generic callback function used for testing purposes.
 */
void generic_callback(char* argv, uint32_t argc) {
}

static Dispatcher_t test_dispatcher;

// Initialize dispatcher at the beginning of the test
static int dispatcher_test_setup(void** state) {
    DispatcherConfig_t cfg = { .delim = " " };
    if(dispatcher_init(&test_dispatcher, cfg) != DISPATCHER_ERR_OK) {
        return -1;
    }
    return 0;
}

// Deinitialize dispatcher after the test
static int dispatcher_test_teardown(void** state) {
    if(dispatcher_deinit(&test_dispatcher) != DISPATCHER_ERR_OK) {
        return -1;
    }
    return 0;
}

/************************ Unit tests ************************/

/* Dispatcher should initialize successfully */
static void test_dispatcher_init_success(void** state) {
    Dispatcher_t dispatcher;
    DispatcherConfig_t cfg = { .delim = " " };
    assert_int_equal(dispatcher_init(&dispatcher, cfg), DISPATCHER_ERR_OK);
}

/* Dispatcher init should fail with NULL context */
static void test_dispatcher_init_null_ctx(void** state) {
    DispatcherConfig_t cfg = { .delim = " " };
    assert_int_equal(dispatcher_init(NULL, cfg), DISPATCHER_ERR_NULL_ARG);
}

/* Dispatcher deinit should return success */
static void test_dispatcher_deinit_success(void** state) {
    assert_int_equal(dispatcher_deinit(&test_dispatcher), DISPATCHER_ERR_OK);
}

/* Dispatcher deinit with NULL context should fail */
static void test_dispatcher_deinit_null_ctx(void** state) {
    assert_int_equal(dispatcher_deinit(NULL), DISPATCHER_ERR_NULL_ARG);
}

/* Adding a command should succeed */
static void test_dispatcher_register_success(void** state) {
    DispatcherCommandDef_t cmd = { .target = "gpio", .action = "set", .callback_ptr = (void*)1 };
    assert_int_equal(dispatcher_register(&test_dispatcher, 0, cmd), DISPATCHER_ERR_OK);
}

/* Adding a command with a NULL context should fail */
static void test_dispatcher_register_null_ctx(void** state) {
    DispatcherCommandDef_t cmd = { .target = "gpio", .action = "set", .callback_ptr = (void*)1 };
    assert_int_equal(dispatcher_register(NULL, 0, cmd), DISPATCHER_ERR_NULL_ARG);
}

/* Adding multiple commands should succeed */
static void test_dispatcher_add_multiple_cmds(void** state) {
    for(uint32_t i = 0; i < 5; i++) {
        DispatcherCommandDef_t cmd = { .target = "cmd", .action = "act", .callback_ptr = (void*)1 };
        assert_int_equal(dispatcher_register(&test_dispatcher, i, cmd), DISPATCHER_ERR_OK);
    }
}

/* Adding a command with an empty action should fail */
static void test_dispatcher_register_empty_action(void** state) {
    DispatcherCommandDef_t cmd = { .target = "gpio", .action = "", .callback_ptr = (void*)1 };
    assert_int_equal(dispatcher_register(&test_dispatcher, (uint32_t)0, cmd), DISPATCHER_ERR_INVALID_ARG);
}

/* Adding a command at an invalid index should fail */
static void test_dispatcher_register_invalid_index(void** state) {
    DispatcherCommandDef_t cmd = { .target = "gpio", .action = "set", .callback_ptr = (void*)1 };
    assert_int_equal(dispatcher_register(&test_dispatcher, DISPATCHER_MAX_CMD_COUNT + 1, cmd), DISPATCHER_ERR_INVALID_ARG);
}


/* Executing a valid command should return success */
static void test_dispatcher_execute_success(void** state) {
    DispatcherCommandDef_t cmd = { .target = "gpio", .action = "set", .callback_ptr = generic_callback };
    dispatcher_register(&test_dispatcher, (uint32_t)0, cmd);
    assert_int_equal(dispatcher_execute(&test_dispatcher, "gpio set 13 1"), DISPATCHER_ERR_OK);
}

/* Executing a NULL buffer should fail */
static void test_dispatcher_execute_null_buf(void** state) {
    assert_int_equal(dispatcher_execute(&test_dispatcher, NULL), DISPATCHER_ERR_NULL_ARG);
}

/* Executing an empty buffer should return error */
static void test_dispatcher_execute_empty_buf(void** state) {
    assert_int_equal(dispatcher_execute(&test_dispatcher, ""), DISPATCHER_ERR_BUF_EMPTY);
}

/* Dispatcher should reject an overly long buffer */
static void test_dispatcher_execute_long_buf(void** state) {
    char long_buf[DISPATCHER_MAX_BUF_SIZE + 1];
    memset(long_buf, 'A', DISPATCHER_MAX_BUF_SIZE);
    long_buf[DISPATCHER_MAX_BUF_SIZE] = '\0';
    assert_int_equal(dispatcher_execute(&test_dispatcher, long_buf), DISPATCHER_ERR_BUF_TOO_LONG);
}

/* Removing a command should succeed */
static void test_dispatcher_deregister_success(void** state) {
    assert_int_equal(dispatcher_deregister(&test_dispatcher, 0), DISPATCHER_ERR_OK);
}

/* Removing a non-existent command should return an error */
static void test_dispatcher_deregister_nonexistent(void** state) {
    assert_int_equal(dispatcher_deregister(&test_dispatcher, DISPATCHER_MAX_CMD_COUNT + 1), DISPATCHER_ERR_INVALID_ARG);
}

/* Removing a command with NULL context should fail */
static void test_dispatcher_deregister_null_ctx(void** state) {
    assert_int_equal(dispatcher_deregister(NULL, 0), DISPATCHER_ERR_NULL_ARG);
}

/* Complex functional test: Add, execute, and remove multiple commands */
static void test_dispatcher_functional_test(void** state) {
    char* target[FUNC_TEST_CMD_COUNT] = { "gpio", "net", "sensor", "server", "client" };
    char* action[FUNC_TEST_CMD_COUNT] = { "run", "stop", "test", "list", "nop" };

    // Add command definitions
    for(uint32_t i = 0; i < FUNC_TEST_CMD_COUNT; i++) {
        DispatcherCommandDef_t cmd = { .callback_ptr = generic_callback };
        memcpy(cmd.target, target[i], strlen(target[i]));
        cmd.target[strlen(target[i])] = '\0';
        memcpy(cmd.action, action[i], strlen(action[i]));
        cmd.action[strlen(action[i])] = '\0';
        assert_int_equal(dispatcher_register(&test_dispatcher, i, cmd), DISPATCHER_ERR_OK);
    }

    // Test executing those commands with additional parameters
    for(uint32_t i = 0; i < FUNC_TEST_CMD_COUNT; i++) {
        char buf[DISPATCHER_MAX_BUF_SIZE];
        snprintf(buf, sizeof(buf), "%s %s p1 p2 p3", target[i], action[i]); // Test also optional parameters
        assert_int_equal(dispatcher_execute(&test_dispatcher, buf), DISPATCHER_ERR_OK);
    }

    // Test removing the commands
    for(uint32_t i = 0; i < FUNC_TEST_CMD_COUNT; i++) {
        assert_int_equal(dispatcher_deregister(&test_dispatcher, i), DISPATCHER_ERR_OK);
    }
}

/************************ Test Runner ************************/
int run_dispatcher_tests(void) {
    const struct CMUnitTest dispatcher_tests[] = {
        cmocka_unit_test(test_dispatcher_init_success),
        cmocka_unit_test(test_dispatcher_init_null_ctx),
        cmocka_unit_test(test_dispatcher_deinit_null_ctx),
        cmocka_unit_test_setup_teardown(test_dispatcher_register_success, dispatcher_test_setup, dispatcher_test_teardown),
        cmocka_unit_test_setup_teardown(test_dispatcher_register_null_ctx, dispatcher_test_setup, dispatcher_test_teardown),
        cmocka_unit_test_setup_teardown(test_dispatcher_add_multiple_cmds, dispatcher_test_setup, dispatcher_test_teardown),
        cmocka_unit_test_setup_teardown(test_dispatcher_register_empty_action, dispatcher_test_setup, dispatcher_test_teardown),
        cmocka_unit_test_setup_teardown(test_dispatcher_register_invalid_index, dispatcher_test_setup, dispatcher_test_teardown),
        cmocka_unit_test_setup_teardown(test_dispatcher_execute_success, dispatcher_test_setup, dispatcher_test_teardown),
        cmocka_unit_test_setup_teardown(test_dispatcher_execute_null_buf, dispatcher_test_setup, dispatcher_test_teardown),
        cmocka_unit_test_setup_teardown(test_dispatcher_execute_empty_buf, dispatcher_test_setup, dispatcher_test_teardown),
        cmocka_unit_test_setup_teardown(test_dispatcher_execute_long_buf, dispatcher_test_setup, dispatcher_test_teardown),
        cmocka_unit_test_setup_teardown(test_dispatcher_deregister_success, dispatcher_test_setup, dispatcher_test_teardown),
        cmocka_unit_test_setup_teardown(test_dispatcher_deregister_null_ctx, dispatcher_test_setup, dispatcher_test_teardown),
        cmocka_unit_test_setup_teardown(test_dispatcher_deregister_nonexistent, dispatcher_test_setup, dispatcher_test_teardown),
        cmocka_unit_test_setup_teardown(test_dispatcher_functional_test, dispatcher_test_setup, dispatcher_test_teardown),
    };
    return cmocka_run_group_tests(dispatcher_tests, NULL, NULL);
}
