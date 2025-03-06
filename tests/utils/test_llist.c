#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
// Cmocka must be included last (!)
#include <cmocka.h>

#include "utils/list.h"

extern ListError_t llist_init();
extern ListError_t llist_push(List_t* ctx, void* data, size_t length);
extern ListNode_t* llist_get_head(List_t* ctx);
extern ListNode_t* llist_get_tail(List_t* ctx);
extern int32_t llist_get_length(List_t* ctx);
extern ListError_t llist_remove(List_t* ctx, const void* data);
extern ListError_t llist_traverse(List_t* ctx, ListError_t (*func)(void* data));
extern ListError_t llist_deinit(List_t* ctx);


/********************* Auxiliary functions *********************/

int cmp(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

static ListError_t add_one(void* data) {
    (*(int*)data) += 1;
    return LIST_ERR_OK;
}

/************************ Test fixtures ************************/

List_t test_ll; // Test Linked List

// Init Linked List and push two integers (15 and -10)
static int llist_test_setup(void** state) {
    llist_init(&test_ll, cmp);
    return 0;
}

// Deinit the Linked List
static int llist_test_teardown(void** state) {
    llist_deinit(&test_ll);
    return 0;
}

/************************ Unit tests ************************/

/* Parameters as expected; llist_init should succeed */
static void test_llist_init_success(void** state) {
    List_t list;
    assert_int_equal(llist_init(&list, cmp), LIST_ERR_OK);
}

/* NULL ctx; llist_init should fail */
static void test_llist_init_null_ctx(void** state) {
    List_t list;
    assert_int_equal(llist_init(&list, NULL), LIST_ERR_NULL_ARGUMENT);
}

/* NULL cmp pointer; llist_init should fail */
static void test_llist_init_null_cmp(void** state) {
    assert_int_equal(llist_init(NULL, cmp), LIST_ERR_NULL_ARGUMENT);
}

/* Parameters as expected; llist_deinit should succeed */
static void test_llist_deinit_success(void** state) {
    List_t list;
    llist_init(&list, cmp);
    assert_int_equal(llist_deinit(&list), LIST_ERR_OK);
}

/* NULL ctx; llist_deinit should fail */
static void test_llist_deinit_null_ctx(void** state) {
    assert_int_equal(llist_deinit(NULL), LIST_ERR_NULL_ARGUMENT);
}

/* Parameters as expected; llist_push should succeed */
static void test_llist_push_success(void** state) {
    List_t list;
    llist_init(&list, cmp);
    int data = 10;
    assert_int_equal(llist_push(&list, &data, sizeof(data)), LIST_ERR_OK);
}

/* NULL ctx; llist_push should fail */
static void test_llist_push_null_ctx(void** state) {
    assert_int_equal(llist_push(NULL, NULL, 1), LIST_ERR_NULL_ARGUMENT);
}

/* zero lenght; llist_push should fail */
static void test_llist_push_zero_length(void** state) {
    List_t list;
    assert_int_equal(llist_push(&list, &list, 0), LIST_ERR_INCORRECT_ARGUMENT);
}

/* Parameters as expected; llist_get_head should return the first element */
static void test_llist_get_head_success(void** state) {
    List_t list;
    llist_init(&list, cmp);
    int data = 10;
    llist_push(&list, &data, sizeof(data));
    assert_int_equal(*(int*)llist_get_head(&list)->data, data);
}

/* NULL ctx; llist_get_head should return NULL */
static void test_llist_get_head_null_ctx(void** state) {
    assert_null(llist_get_head(NULL));
}

/* Empty list; llist_get_head should return NULL */
static void test_llist_get_head_empty(void** state) {
    List_t list;
    llist_init(&list, cmp);
    assert_null(llist_get_head(&list));
}

/* Parameters as expected; llist_get_tail should return the last element */
static void test_llist_get_tail_success(void** state) {
    List_t list;
    llist_init(&list, cmp);
    int head = 10, tail = 20;
    llist_push(&list, &head, sizeof(head));
    llist_push(&list, &tail, sizeof(tail));
    assert_int_equal(*(int*)llist_get_tail(&list)->data, tail);
}

/* NULL ctx; llist_get_tail should return NULL */
static void test_llist_get_tail_null_ctx(void** state) {
    assert_null(llist_get_tail(NULL));
}

/* Empty list; llist_get_tail should return NULL */
static void test_llist_get_tail_empty(void** state) {
    List_t list;
    llist_init(&list, cmp);
    assert_null(llist_get_tail(&list));
}

/* Parameters as expected; llist_get_length should return correct length */
static void test_llist_get_length_success(void** state) {
    List_t list;
    llist_init(&list, cmp);
    int data = 10;
    llist_push(&list, &data, sizeof(data));
    assert_int_equal(llist_get_length(&list), 1);
}

/* NULL ctx; llist_get_length should return -1 */
static void test_llist_get_length_null_ctx(void** state) {
    assert_int_equal(llist_get_length(NULL), -1);
}

/* Empty list; llist_get_length should return 0 */
static void test_llist_get_length_empty(void** state) {
    List_t list;
    llist_init(&list, cmp);
    assert_int_equal(llist_get_length(&list), 0);
}

/* Parameters as expected; llist_remove should remove an existing element */
static void test_llist_remove_success(void** state) {
    List_t list;
    llist_init(&list, cmp);
    int data = 10;
    llist_push(&list, &data, sizeof(data));
    assert_int_equal(llist_remove(&list, &data), LIST_ERR_OK);
    assert_int_equal(llist_get_length(&list), 0);
}

/* NULL ctx; llist_remove should fail */
static void test_llist_remove_null_ctx(void** state) {
    int data = 10;
    assert_int_equal(llist_remove(NULL, &data), LIST_ERR_NULL_ARGUMENT);
}

/* Removing a non-existent element; should return LIST_ERR_OK but not modify the list */
static void test_llist_remove_nonexistent(void** state) {
    List_t list;
    llist_init(&list, cmp);
    int data1 = 10, data2 = 20;
    llist_push(&list, &data1, sizeof(data1));
    assert_int_equal(llist_remove(&list, &data2), LIST_ERR_OK);
    assert_int_equal(llist_get_length(&list), 1);
}

/* Parameters as expected; llist_traverse should apply function to all elements */
static void test_llist_traverse_success(void** state) {
    List_t list;
    llist_init(&list, cmp);
    int data = 10;
    llist_push(&list, &data, sizeof(data));
    llist_traverse(&list, add_one);
    assert_int_equal(*(int*)llist_get_head(&list)->data, 11);
}

/* NULL ctx; llist_traverse should fail */
static void test_llist_traverse_null_ctx(void** state) {
    assert_int_equal(llist_traverse(NULL, add_one), LIST_ERR_NULL_ARGUMENT);
}

/* NULL traverse function; llist_traverse should fail */
static void test_llist_traverse_null_func(void** state) {
    List_t list;
    assert_int_equal(llist_traverse(&list, NULL), LIST_ERR_NULL_ARGUMENT);
}

/* Functional test of the llist library focused on pushing new nodes */
static void test_llist_repeated_pushes(void** state) {
    int result = 1;
    int nodes_count = 20;
    int data[nodes_count];
    for(int node = 0; node < nodes_count; node++) {
        data[node] = node * 2 + 2;
    }

    // Push all 'nodes_count' nodes to the Linked List
    for(int node = 0; node < nodes_count; node++) {
        ListError_t push_result = llist_push(&test_ll, &data[node], sizeof(data[node]));
        result = result && (push_result == LIST_ERR_OK);
    }

    // Check all nodes
    ListNode_t* current = llist_get_head(&test_ll);
    for(int node = 0; node < nodes_count; node++) {
        if(!current) {
            result = 0;
            break;
        }
        int comparison_result = (*(int*)(current->data) == data[node]);
        result = result && comparison_result;
        current = current->next;
    }

    assert_true(result);
}

/* Functional test of the llist library focused on pushing and removing nodes */
static void test_llist_repeated_removes(void** state) {
    int result = 1;
    int nodes_count = 20;
    int data[nodes_count];
    for(int node = 0; node < nodes_count; node++) {
        data[node] = node * 2 + 2;
    }

    // Push all 'nodes_count' nodes to the Linked List
    for(int node = 0; node < nodes_count; node++) {
        ListError_t push_result = llist_push(&test_ll, &data[node], sizeof(data[node]));
        result = result && (push_result == LIST_ERR_OK);
    }

    // Remove first half of nodes
    for(int node = 0; node < (nodes_count / 2); node++) {
        ListError_t push_result = llist_remove(&test_ll, &data[node]);
        result = result && (push_result == LIST_ERR_OK);
    }

    // Check all nodes (only second half of the initial data array should be in the ll)
    ListNode_t* current = llist_get_head(&test_ll);
    for(int node = (nodes_count / 2); node < nodes_count; node++) {
        if(!current) {
            result = 0;
            break;
        }
        int comparison_result = (*(int*)(current->data) == data[node]);
        result = result && comparison_result;
        current = current->next;
    }

    assert_true(result);
}

int run_llist_tests(void) {
    const struct CMUnitTest llist_tests[] = {
        cmocka_unit_test(test_llist_init_success),
        cmocka_unit_test(test_llist_init_null_ctx),
        cmocka_unit_test(test_llist_init_null_cmp),
        cmocka_unit_test(test_llist_deinit_success),
        cmocka_unit_test(test_llist_deinit_null_ctx),
        cmocka_unit_test(test_llist_push_success),
        cmocka_unit_test(test_llist_push_null_ctx),
        cmocka_unit_test(test_llist_push_zero_length),
        cmocka_unit_test(test_llist_get_head_success),
        cmocka_unit_test(test_llist_get_head_null_ctx),
        cmocka_unit_test(test_llist_get_head_empty),
        cmocka_unit_test(test_llist_get_tail_success),
        cmocka_unit_test(test_llist_get_tail_null_ctx),
        cmocka_unit_test(test_llist_get_tail_empty),
        cmocka_unit_test(test_llist_get_length_success),
        cmocka_unit_test(test_llist_get_length_null_ctx),
        cmocka_unit_test(test_llist_get_length_empty),
        cmocka_unit_test(test_llist_remove_success),
        cmocka_unit_test(test_llist_remove_null_ctx),
        cmocka_unit_test(test_llist_remove_nonexistent),
        cmocka_unit_test(test_llist_traverse_success),
        cmocka_unit_test(test_llist_traverse_null_ctx),
        cmocka_unit_test(test_llist_traverse_null_func),
        cmocka_unit_test_setup_teardown(test_llist_repeated_pushes, llist_test_setup, llist_test_teardown),
        cmocka_unit_test_setup_teardown(test_llist_repeated_removes, llist_test_setup, llist_test_teardown),
    };
    return cmocka_run_group_tests(llist_tests, NULL, NULL);
}
