#include "utils/list.h"

#include <string.h> // For: memmove

#include "utils/common.h"
#include "utils/log.h"

ListError_t llist_push(List_t* this, void* data, size_t length);
ListNode_t* llist_get_head(List_t* this);
ListNode_t* llist_get_tail(List_t* this);
int32_t llist_get_length(List_t* this);
ListError_t llist_remove(List_t* this, const void* data);
ListError_t llist_traverse(List_t* this, ListError_t (*func)(void* data));
ListError_t llist_destroy(List_t** this);

STATIC inline void llist_init_functions(List_t* ll) {
    ll->push = llist_push;
    ll->get_head = llist_get_head;
    ll->get_tail = llist_get_tail;
    ll->get_length = llist_get_length;
    ll->remove = llist_remove;
    ll->traverse = llist_traverse;
    ll->destroy = llist_destroy;
}

List_t* llist_create(int (*cmp)(const void* a, const void* b)) {
    if(!cmp) {
        log_error("NULL ptr provided to llist_create");
        return NULL;
    }

    List_t* ll = (List_t*)malloc(sizeof(List_t));
    if(!ll) {
        log_error("malloc() returned NULL on new List_t instance creation");
        return NULL;
    }

    /* Populate data in the struct (cfg) and assign function pointers */
    ll->head = NULL;
    ll->compare_data = cmp;
    llist_init_functions(ll);

    // Initialize mutex for protecting ll-related critical sections
    pthread_mutex_init(&ll->lock, NULL);

    return ll;
}

ListError_t llist_push(List_t* this, void* data, size_t length) {
    if(!this || !data) {
        log_error("NULL ptr provided to llist_push");
        return LIST_ERR_NULL_ARGUMENT;
    }

    // Allocate and initialize memory for a new node and for its data
    ListNode_t* new_node = (ListNode_t*)calloc(1, sizeof(ListNode_t));
    if(!new_node) {
        free(new_node);
        log_error("malloc() returned NULL on new node creation");
        return LIST_ERR_MALLOC_FAILED;
    }
    new_node->data = calloc(1, length);
    if(!(new_node->data)) {
        free(new_node->data);
        free(new_node);
        log_error("malloc() returned NULL when allocating memory for new_node->data");
        return LIST_ERR_MALLOC_FAILED;
    }
    memmove(new_node->data, data, length);
    new_node->next = NULL;

    // Add the new node to the end of the list (Critical section!)
    int ret = pthread_mutex_lock(&this->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return LIST_ERR_MULTITHREAD_ISSUE;
    }

    for(ListNode_t** node = &this->head;; node = &((*node)->next)) {
        if(*node == NULL) {
            *node = new_node;
            break;
        }
    }

    ret = pthread_mutex_unlock(&this->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return LIST_ERR_MULTITHREAD_ISSUE;
    }

    return LIST_ERR_OK;
}

ListNode_t* llist_get_head(List_t* this) {
    if(!this) {
        log_error("NULL ptr provided to llist_get_head");
        return NULL;
    }

    // Retrieve the head node (or NULL if the list is empty) (Critical section!)
    int ret = pthread_mutex_lock(&this->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return NULL;
    }

    ListNode_t* head = this->head;

    ret = pthread_mutex_unlock(&this->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return NULL;
    }

    return head;
}

ListNode_t* llist_get_tail(List_t* this) {
    if(!this) {
        log_error("NULL ptr provided to llist_get_tail");
        return NULL;
    }

    // Retrieve data from the tail node (or NULL if the list is empty) (Critical section!)
    ListNode_t* tail;
    int ret = pthread_mutex_lock(&this->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return NULL;
    }

    if(this->head == NULL) {
        tail = NULL;
    } else {
        for(ListNode_t* node = this->head;; node = node->next) {
            if(node->next == NULL) {
                tail = node;
                break;
            }
        }
    }

    ret = pthread_mutex_unlock(&this->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return NULL;
    }

    return tail;
}

int32_t llist_get_length(List_t* this) {
    if(!this) {
        log_error("NULL ptr provided to llist_get_length");
        return -1;
    }

    // Count the number of nodes in the Linked List (Critical section!)
    int32_t node_count = 0;
    int ret = pthread_mutex_lock(&this->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return -1;
    }

    for(ListNode_t* node = this->head; node != NULL; node = node->next) {
        ++node_count;
    }

    ret = pthread_mutex_unlock(&this->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return -1;
    }

    return node_count;
}

ListError_t llist_remove(List_t* this, const void* data) {
    if(!this) {
        log_error("NULL ptr provided to llist_remove");
        return LIST_ERR_NULL_ARGUMENT;
    }

    // Find and remove the first node whose data is equal to the one provided in args (Critical section!)
    int ret = pthread_mutex_lock(&this->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return LIST_ERR_MULTITHREAD_ISSUE;
    }

    ListNode_t* to_remove = NULL;
    if(this->head) { // Proceed only if the list is not empty
        // Check if the node to remove is the head
        if((this->compare_data(data, this->head->data) == 0)) {
            to_remove = this->head;
            this->head = this->head->next;
        } else {
            // Otherwise traverse through all nodes and compare their 'data' to the one provided in args
            ListNode_t** node = &(this->head);
            while(*node != NULL && (*node)->next != NULL) {
                if(this->compare_data(data, (*node)->next->data) == 0) {
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

    ret = pthread_mutex_unlock(&this->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return LIST_ERR_MULTITHREAD_ISSUE;
    }

    return LIST_ERR_OK;
}

ListError_t llist_traverse(List_t* this, ListError_t (*func)(void* data)) {
    if(!this) {
        log_error("NULL ptr provided to llist_traverse");
        return LIST_ERR_NULL_ARGUMENT;
    }

    // Apply func to the data of each node (Critical section!)
    ListError_t err = LIST_ERR_OK;
    int ret = pthread_mutex_lock(&this->lock);
    if(ret != 0) {
        log_error("pthread_mutex_lock() returned %d", ret);
        return LIST_ERR_MULTITHREAD_ISSUE;
    }

    for(ListNode_t* node = this->head; node != NULL; node = node->next) {
        err = func(node->data);
        if(err != LIST_ERR_OK) {
            break;
        }
    }

    ret = pthread_mutex_unlock(&this->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return LIST_ERR_MULTITHREAD_ISSUE;
    }

    return err;
}

ListError_t llist_destroy(List_t** this) {
    if(!this || !*this) {
        log_error("NULL ptr provided to llist_destroy");
        return LIST_ERR_NULL_ARGUMENT;
    }

    int ret = pthread_mutex_lock(&(*this)->lock);
    if(ret != 0) {
        pthread_mutex_unlock(&(*this)->lock);
        log_error("pthread_mutex_lock() returned %d", ret);
        return LIST_ERR_MULTITHREAD_ISSUE;
    }

    // Free the memory allocated for each node (if the list is not empty)
    if((*this)->head != NULL) {
        for(ListNode_t* node = (*this)->head; node != NULL;) {
            ListNode_t* next_node = node->next;
            free(node->data);
            free(node);
            node = next_node;
        }
    }

    ret = pthread_mutex_unlock(&(*this)->lock);
    if(ret != 0) {
        log_error("pthread_mutex_unlock() returned %d", ret);
        return LIST_ERR_MULTITHREAD_ISSUE;
    }

    // Destroy the lock (TODO: Improve mutex destroy mechanism, e.g. add a global protection variable)
    ret = pthread_mutex_destroy(&(*this)->lock);
    if(ret != 0) {
        log_error("pthread_mutex_destroy() returned %d", ret);
        return LIST_ERR_MULTITHREAD_ISSUE;
    }

    // Free the memory allocated for this List_t instance
    free(*this);
    *this = NULL;

    return LIST_ERR_OK;
}
