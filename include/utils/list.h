/**
 * @file list.h
 * @brief A simple generic singly-linked list
 *
 * @note Designed to provide thread-safe functionality (MT-Safe)
 *
 */

#ifndef __LIST_H__
#define __LIST_H__

#include <pthread.h> // For: pthread_mutex_t
#include <stdint.h>  // For: std types
#include <stdlib.h>  // For: size_t

typedef enum {
    LIST_ERR_OK = 0x00,          /**< Operation finished successfully */
    LIST_ERR_NODE_NOT_FOUND,     /**< Error: No matching node found */
    LIST_ERR_NULL_ARGUMENT,      /**< Error: NULL pointer passed as argument */
    LIST_ERR_MALLOC_FAILURE,     /**< Error: Dynamic memory allocation failed */
    LIST_ERR_INCORRECT_ARGUMENT, /**< Error: Incorrect argument provided */
    LIST_ERR_PTHREAD_FAILURE,    /**< Error: Thread-safety related issue */
    LIST_ERR_GENERIC             /**< Error: Generic error */
} ListError_t;

// A single node in the linked list
typedef struct ListNode {
    void* data;
    struct ListNode* next;
} ListNode_t;

/**
 * @brief Linked List structure
 * Include the ptr to ll head, ptr to compare function, mutex for protecting read/write operations, and pointers to member functions.
 */
typedef struct List {
    ListNode_t* head;     // First node in the ll
    pthread_mutex_t lock; // Mutex for protecting critical sections that read/write to the ll
    int (*compare_data)(const void*, const void*); // Function for comparing data in nodes

    /**
     * @brief Add a new node with containing 'data' at the end of the list
     * @param[in]  ctx  Pointer to the Linked List instance
     * @param[in]  data  Pointer to the 'data' to be copied into the new node
     * @param[in]  length Size in bytes of the data to be stored
     * @return LIST_ERR_OK on success, LIST_ERR_NULL_ARGUMENT or LIST_ERR_MALLOC_FAILURE otherwise
     */
    ListError_t (*push)(struct List* ctx, void* data, size_t length);

    /**
     * @brief Retrieve the head node
     * @param[in]  ctx  Pointer to the Linked List instance
     * @return Pointer to the head node, NULL if the list is empty
     */
    ListNode_t* (*get_head)(struct List* ctx);

    /**
     * @brief Retrieve the tail node
     * @param[in]  ctx  Pointer to the Linked List instance
     * @return Pointer to the tail node, NULL if the list is empty
     */
    ListNode_t* (*get_tail)(struct List* ctx);

    /**
     * @brief Get the number of nodes
     * @param[in]  ctx  Pointer to the Linked List instance
     * @return Number of nodes in this linked list on success or -1 on failure
     */
    int32_t (*get_length)(struct List* ctx);

    /**
     * @brief Remove the first node whose data is equal to 'data'
     * @param[in]  ctx  Pointer to the Linked List instance
     * @param[in]  data  Pointer to data to look for
     * @return LIST_ERR_OK on success, LIST_ERR_NULL_ARGUMENT or LIST_ERR_PTHREAD_FAILURE otherwise
     */
    ListError_t (*remove)(struct List* ctx, const void* data);

    /**
     * @brief Traverse through all nodes and apply func to the data of each one
     * @param[in]  ctx  Pointer to the Linked List instance
     * @param[in]  func  Pointer to function that should be called for each node
     * @return LIST_ERR_OK on success, LIST_ERR_NULL_ARGUMENT or LIST_ERR_PTHREAD_FAILURE otherwise
     * @note This function returns if at any point func() does not return LIST_ERR_OK
     */
    ListError_t (*traverse)(struct List* ctx, ListError_t (*func)(void* data));

    /**
     * @brief Deinitialize this instance of List_t and free used resources
     * @param[in]  ctx  Pointer to the Linked List instance
     * @return LIST_ERR_OK on success, LIST_ERR_PTHREAD_FAILURE or LIST_ERR_NULL_ARGUMENT otherwise
     */
    ListError_t (*deinit)(struct List* ctx);
} List_t;

/**
 * @brief Initialize a new Linked list instance
 * @param[in, out]  ctx  Pointer to the List instance
 * @param[in] compare Function pointer for comparing node data.
 * @return LIST_ERR_OK on success, LIST_ERR_NULL_ARGUMENT otherwise
 */
ListError_t llist_init(List_t* ctx, int (*cmp)(const void* a, const void* b));

#endif // __LIST_H__
