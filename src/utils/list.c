#include "utils/list.h"

#include <string.h> // For: memmove

#include "utils/common.h"
#include "utils/log.h"

STATIC ListError_t llist_push(List_t* ctx, void* data, size_t length);
STATIC ListNode_t* llist_get_head(List_t* ctx);
STATIC ListNode_t* llist_get_tail(List_t* ctx);
STATIC int32_t llist_get_length(List_t* ctx);
STATIC ListError_t llist_remove(List_t* ctx, const void* data);
STATIC ListError_t llist_traverse(List_t* ctx, ListError_t (*func)(void* data));
STATIC ListError_t llist_deinit(List_t* ctx);


STATIC ListError_t llist_push(List_t* ctx, void* data, size_t length) {
    if(!ctx || !data) {
        log_error("NULL ptr provided to llist_push");
        return LIST_ERR_NULL_ARGUMENT;
    }

    // Allocate and initialize memory for a new node and for its data
    ListNode_t* new_node = (ListNode_t*)calloc(1, sizeof(ListNode_t));
    if(!new_node) {
        free(new_node);
        log_error("malloc() returned NULL on new node creation");
        return LIST_ERR_MALLOC_FAILURE;
    }
    new_node->data = calloc(1, length);
    if(!(new_node->data)) {
        free(new_node->data);
        free(new_node);
        log_error("malloc() returned NULL when allocating memory for new_node->data");
        return LIST_ERR_MALLOC_FAILURE;
    }
    memmove(new_node->data, data, length);
    new_node->next = NULL;

    // Add the new node to the end of the list (Critical section!)
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return LIST_ERR_PTHREAD_FAILURE;
    }
    log_debug("llist lock taken");

    for(ListNode_t** node = &ctx->head;; node = &((*node)->next)) {
        if(*node == NULL) {
            *node = new_node;
            break;
        }
    }

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return LIST_ERR_PTHREAD_FAILURE;
    }
    log_debug("llist lock released");

    return LIST_ERR_OK;
}

STATIC ListNode_t* llist_get_head(List_t* ctx) {
    if(!ctx) {
        log_error("NULL ptr provided to llist_get_head");
        return NULL;
    }

    // Retrieve the head node (or NULL if the list is empty) (Critical section!)
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return NULL;
    }

    ListNode_t* head = ctx->head;

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return NULL;
    }

    return head;
}

STATIC ListNode_t* llist_get_tail(List_t* ctx) {
    if(!ctx) {
        log_error("NULL ptr provided to llist_get_tail");
        return NULL;
    }

    // Retrieve data from the tail node (or NULL if the list is empty) (Critical section!)
    ListNode_t* tail;
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return NULL;
    }

    if(ctx->head == NULL) {
        tail = NULL;
    } else {
        for(ListNode_t* node = ctx->head;; node = node->next) {
            if(node->next == NULL) {
                tail = node;
                break;
            }
        }
    }

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return NULL;
    }

    return tail;
}

STATIC int32_t llist_get_length(List_t* ctx) {
    if(!ctx) {
        log_error("NULL ptr provided to llist_get_length");
        return -1;
    }

    // Count the number of nodes in the Linked List (Critical section!)
    int32_t node_count = 0;
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return -1;
    }

    for(ListNode_t* node = ctx->head; node != NULL; node = node->next) {
        ++node_count;
    }

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return -1;
    }

    return node_count;
}

STATIC ListError_t llist_remove(List_t* ctx, const void* data) {
    if(!ctx) {
        log_error("NULL ptr provided to llist_remove");
        return LIST_ERR_NULL_ARGUMENT;
    }

    // Find and remove the first node whose data is equal to the one provided in args (Critical section!)
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return LIST_ERR_PTHREAD_FAILURE;
    }
    log_debug("llist lock taken");

    ListNode_t* to_remove = NULL;
    if(ctx->head) { // Proceed only if the list is not empty
        // Check if the node to remove is the head
        if((ctx->compare_data(data, ctx->head->data) == 0)) {
            to_remove = ctx->head;
            ctx->head = ctx->head->next;
        } else {
            // Otherwise traverse through all nodes and compare their 'data' to the one provided in args
            ListNode_t** node = &(ctx->head);
            while(*node != NULL && (*node)->next != NULL) {
                if(ctx->compare_data(data, (*node)->next->data) == 0) {
                    to_remove = (*node)->next;
                    (*node)->next = (*node)->next->next;
                    break;
                }
                node = &(*node)->next;
            }
        }
    }

    // Clean the resources used by the removed node
    if(to_remove) {
        free(to_remove->data);
        free(to_remove);
    }

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return LIST_ERR_PTHREAD_FAILURE;
    }
    log_debug("llist lock released");

    return LIST_ERR_OK;
}

STATIC ListError_t llist_traverse(List_t* ctx, ListError_t (*func)(void* data)) {
    if(!ctx) {
        log_error("NULL ptr provided to llist_traverse");
        return LIST_ERR_NULL_ARGUMENT;
    }

    // Apply func to the data of each node (Critical section!)
    ListError_t err = LIST_ERR_OK;
    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return LIST_ERR_PTHREAD_FAILURE;
    }
    log_debug("llist lock taken");

    for(ListNode_t* node = ctx->head; node != NULL; node = node->next) {
        err = func(node->data);
        if(err != LIST_ERR_OK) {
            break;
        }
    }

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return LIST_ERR_PTHREAD_FAILURE;
    }
    log_debug("llist lock released");

    return err;
}

STATIC ListError_t llist_deinit(List_t* ctx) {
    if(!ctx) {
        log_error("NULL ptr provided to llist_destroy");
        return LIST_ERR_NULL_ARGUMENT;
    }

    int ret = pthread_mutex_lock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return LIST_ERR_PTHREAD_FAILURE;
    }

    // Free the memory allocated for each node (if the list is not empty)
    if(ctx->head != NULL) {
        for(ListNode_t* node = ctx->head; node != NULL;) {
            ListNode_t* next_node = node->next;
            free(node->data);
            free(node);
            node = next_node;
        }
    }

    ret = pthread_mutex_unlock(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return LIST_ERR_PTHREAD_FAILURE;
    }
    log_debug("llist lock released");

    // Destroy the lock (@TODO: Improve mutex destroy mechanism, e.g. add a global protection variable)
    ret = pthread_mutex_destroy(&ctx->lock);
    if(ret != 0) {
        log_error("pthread_mutex_destroy() returned %d", ret);
        return LIST_ERR_PTHREAD_FAILURE;
    }

    return LIST_ERR_OK;
}

ListError_t llist_init(List_t* ctx, int (*cmp)(const void* a, const void* b)) {
    if(!cmp || !ctx) {
        return LIST_ERR_NULL_ARGUMENT;
    }

    /* Populate data in the struct (cfg) and assign function pointers */
    ctx->head = NULL;
    ctx->compare_data = cmp;
    ctx->push = llist_push;
    ctx->get_head = llist_get_head;
    ctx->get_tail = llist_get_tail;
    ctx->get_length = llist_get_length;
    ctx->remove = llist_remove;
    ctx->traverse = llist_traverse;
    ctx->deinit = llist_deinit;

    // Initialize mutex for protecting ll-related critical sections
    int err = pthread_mutex_init(&ctx->lock, NULL);
    if(err != 0) {
        log_error("pthread_mutex_init() returned %d", err);
        return LIST_ERR_PTHREAD_FAILURE;
    }
    
    return LIST_ERR_OK;
}
