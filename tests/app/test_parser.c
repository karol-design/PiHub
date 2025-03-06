#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// Cmocka must be included last (!)
#include <cmocka.h>

#include "app/parser.h"

#define FUNC_TEST_CMD_COUNT 5 // number of commands to be tested in a functional test

extern ParserError_t parser_init(Parser_t* ctx, const ParserConfig_t cfg);
extern ParserError_t parser_add_cmd(Parser_t* ctx, const uint32_t id, const ParserCommandDef_t cmd);
extern ParserError_t parser_remove_cmd(Parser_t* ctx, const uint32_t id);
extern ParserError_t parser_execute(Parser_t* ctx, const char* buf);
extern ParserError_t parser_deinit(Parser_t* ctx);

/********************* Auxiliary functions *********************/

/**
 * @brief A generic callback function used for testing purposes.
 */
void generic_callback(char* argv, uint32_t argc) {
}

static Parser_t test_parser;

// Initialize parser at the beginning of the test
static int parser_test_setup(void** state) {
    ParserConfig_t cfg = { .delim = " " };
    if(parser_init(&test_parser, cfg) != PARSER_ERR_OK) {
        return -1;
    }
    return 0;
}

// Deinitialize parser after the test
static int parser_test_teardown(void** state) {
    if(parser_deinit(&test_parser) != PARSER_ERR_OK) {
        return -1;
    }
    return 0;
}

/************************ Unit tests ************************/

/* Parser should initialize successfully */
static void test_parser_init_success(void** state) {
    Parser_t parser;
    ParserConfig_t cfg = { .delim = " " };
    assert_int_equal(parser_init(&parser, cfg), PARSER_ERR_OK);
}

/* Parser init should fail with NULL context */
static void test_parser_init_null_ctx(void** state) {
    ParserConfig_t cfg = { .delim = " " };
    assert_int_equal(parser_init(NULL, cfg), PARSER_ERR_NULL_ARG);
}

/* Parser deinit should return success */
static void test_parser_deinit_success(void** state) {
    assert_int_equal(parser_deinit(&test_parser), PARSER_ERR_OK);
}

/* Parser deinit with NULL context should fail */
static void test_parser_deinit_null_ctx(void** state) {
    assert_int_equal(parser_deinit(NULL), PARSER_ERR_NULL_ARG);
}

/* Adding a command should succeed */
static void test_parser_add_cmd_success(void** state) {
    ParserCommandDef_t cmd = { .target = "gpio", .action = "set", .callback_ptr = (void*)1 };
    assert_int_equal(parser_add_cmd(&test_parser, 0, cmd), PARSER_ERR_OK);
}

/* Adding a command with a NULL context should fail */
static void test_parser_add_cmd_null_ctx(void** state) {
    ParserCommandDef_t cmd = { .target = "gpio", .action = "set", .callback_ptr = (void*)1 };
    assert_int_equal(parser_add_cmd(NULL, 0, cmd), PARSER_ERR_NULL_ARG);
}

/* Adding multiple commands should succeed */
static void test_parser_add_multiple_cmds(void** state) {
    for(uint32_t i = 0; i < 5; i++) {
        ParserCommandDef_t cmd = { .target = "cmd", .action = "act", .callback_ptr = (void*)1 };
        assert_int_equal(parser_add_cmd(&test_parser, i, cmd), PARSER_ERR_OK);
    }
}

/* Adding a command with an empty action should fail */
static void test_parser_add_cmd_empty_action(void** state) {
    ParserCommandDef_t cmd = { .target = "gpio", .action = "", .callback_ptr = (void*)1 };
    assert_int_equal(parser_add_cmd(&test_parser, (uint32_t)0, cmd), PARSER_ERR_INVALID_ARG);
}

/* Adding a command at an invalid index should fail */
static void test_parser_add_cmd_invalid_index(void** state) {
    ParserCommandDef_t cmd = { .target = "gpio", .action = "set", .callback_ptr = (void*)1 };
    assert_int_equal(parser_add_cmd(&test_parser, PARSER_MAX_CMD_COUNT + 1, cmd), PARSER_ERR_INVALID_ARG);
}


/* Executing a valid command should return success */
static void test_parser_execute_success(void** state) {
    ParserCommandDef_t cmd = { .target = "gpio", .action = "set", .callback_ptr = generic_callback };
    parser_add_cmd(&test_parser, (uint32_t)0, cmd);
    assert_int_equal(parser_execute(&test_parser, "gpio set 13 1"), PARSER_ERR_OK);
}

/* Executing a NULL buffer should fail */
static void test_parser_execute_null_buf(void** state) {
    assert_int_equal(parser_execute(&test_parser, NULL), PARSER_ERR_NULL_ARG);
}

/* Executing an empty buffer should return error */
static void test_parser_execute_empty_buf(void** state) {
    assert_int_equal(parser_execute(&test_parser, ""), PARSER_ERR_BUF_EMPTY);
}

/* Parser should reject an overly long buffer */
static void test_parser_execute_long_buf(void** state) {
    char long_buf[PARSER_MAX_BUF_SIZE + 1];
    memset(long_buf, 'A', PARSER_MAX_BUF_SIZE);
    long_buf[PARSER_MAX_BUF_SIZE] = '\0';
    assert_int_equal(parser_execute(&test_parser, long_buf), PARSER_ERR_BUF_TOO_LONG);
}

/* Removing a command should succeed */
static void test_parser_remove_cmd_success(void** state) {
    assert_int_equal(parser_remove_cmd(&test_parser, 0), PARSER_ERR_OK);
}

/* Removing a non-existent command should return an error */
static void test_parser_remove_cmd_nonexistent(void** state) {
    assert_int_equal(parser_remove_cmd(&test_parser, PARSER_MAX_CMD_COUNT + 1), PARSER_ERR_INVALID_ARG);
}

/* Removing a command with NULL context should fail */
static void test_parser_remove_cmd_null_ctx(void** state) {
    assert_int_equal(parser_remove_cmd(NULL, 0), PARSER_ERR_NULL_ARG);
}

/* Complex functional test: Add, execute, and remove multiple commands */
static void test_parser_functional_test(void** state) {
    char* target[FUNC_TEST_CMD_COUNT] = { "gpio", "net", "sensor", "server", "client" };
    char* action[FUNC_TEST_CMD_COUNT] = { "run", "stop", "test", "list", "nop" };

    // Add command definitions
    for(uint32_t i = 0; i < FUNC_TEST_CMD_COUNT; i++) {
        ParserCommandDef_t cmd = { .callback_ptr = generic_callback };
        memcpy(cmd.target, target[i], strlen(target[i]));
        cmd.target[strlen(target[i])] = '\0';
        memcpy(cmd.action, action[i], strlen(action[i]));
        cmd.action[strlen(action[i])] = '\0';
        assert_int_equal(parser_add_cmd(&test_parser, i, cmd), PARSER_ERR_OK);
    }

    // Test executing those commands with additional parameters
    for(uint32_t i = 0; i < FUNC_TEST_CMD_COUNT; i++) {
        char buf[PARSER_MAX_BUF_SIZE];
        snprintf(buf, sizeof(buf), "%s %s p1 p2 p3", target[i], action[i]); // Test also optional parameters
        assert_int_equal(parser_execute(&test_parser, buf), PARSER_ERR_OK);
    }

    // Test removing the commands
    for(uint32_t i = 0; i < FUNC_TEST_CMD_COUNT; i++) {
        assert_int_equal(parser_remove_cmd(&test_parser, i), PARSER_ERR_OK);
    }
}

/************************ Test Runner ************************/
int run_parser_tests(void) {
    const struct CMUnitTest parser_tests[] = {
        cmocka_unit_test(test_parser_init_success),
        cmocka_unit_test(test_parser_init_null_ctx),
        cmocka_unit_test(test_parser_deinit_null_ctx),
        cmocka_unit_test_setup_teardown(test_parser_add_cmd_success, parser_test_setup, parser_test_teardown),
        cmocka_unit_test_setup_teardown(test_parser_add_cmd_null_ctx, parser_test_setup, parser_test_teardown),
        cmocka_unit_test_setup_teardown(test_parser_add_multiple_cmds, parser_test_setup, parser_test_teardown),
        cmocka_unit_test_setup_teardown(test_parser_add_cmd_empty_action, parser_test_setup, parser_test_teardown),
        cmocka_unit_test_setup_teardown(test_parser_add_cmd_invalid_index, parser_test_setup, parser_test_teardown),
        cmocka_unit_test_setup_teardown(test_parser_execute_success, parser_test_setup, parser_test_teardown),
        cmocka_unit_test_setup_teardown(test_parser_execute_null_buf, parser_test_setup, parser_test_teardown),
        cmocka_unit_test_setup_teardown(test_parser_execute_empty_buf, parser_test_setup, parser_test_teardown),
        cmocka_unit_test_setup_teardown(test_parser_execute_long_buf, parser_test_setup, parser_test_teardown),
        cmocka_unit_test_setup_teardown(test_parser_remove_cmd_success, parser_test_setup, parser_test_teardown),
        cmocka_unit_test_setup_teardown(test_parser_remove_cmd_null_ctx, parser_test_setup, parser_test_teardown),
        cmocka_unit_test_setup_teardown(test_parser_remove_cmd_nonexistent, parser_test_setup, parser_test_teardown),
        cmocka_unit_test_setup_teardown(test_parser_functional_test, parser_test_setup, parser_test_teardown),
    };
    return cmocka_run_group_tests(parser_tests, NULL, NULL);
}
