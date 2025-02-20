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
    LIST_ERR_MALLOC_FAILED,      /**< Error: Dynamic memory allocation failed */
    LIST_ERR_INCORRECT_ARGUMENT, /**< Error: Incorrect argument provided */
    LIST_ERR_MULTITHREAD_ISSUE,  /**< Error: Thread-safety related issue */
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
     * @param[in]  this  Pointer to the Linked List instance
     * @param[in]  data  Pointer to the 'data' to be copied into the new node
     * @param[in]  lenght Size in bytes of the data to be stored
     * @return LIST_ERR_OK on success, LIST_ERR_NULL_ARGUMENT or LIST_ERR_MALLOC_FAILED otherwise
     */
    ListError_t (*push)(struct List* this, void* data, size_t length);

    /**
     * @brief Retrieve the head node
     * @param[in]  this  Pointer to the Linked List instance
     * @return Pointer to the head node, NULL if the list is empty
     */
    ListNode_t* (*get_head)(struct List* this);

    /**
     * @brief Retrieve the tail node
     * @param[in]  this  Pointer to the Linked List instance
     * @return Pointer to the tail node, NULL if the list is empty
     */
    ListNode_t* (*get_tail)(struct List* this);

    /**
     * @brief Get the number of nodes
     * @param[in]  this  Pointer to the Linked List instance
     * @return Number of nodes in this linked list on success or -1 on failure
     */
    int32_t (*get_length)(struct List* this);

    /**
     * @brief Remove the first node whose data is equal to 'data'
     * @param[in]  this  Pointer to the Linked List instance
     * @param[in]  data  Pointer to data to look for
     * @return LIST_ERR_OK on success, LIST_ERR_NULL_ARGUMENT or LIST_ERR_MULTITHREAD_ISSUE otherwise
     */
    ListError_t (*remove)(struct List* this, const void* data);

    /**
     * @brief Traverse through all nodes and apply func to the data of each one
     * @param[in]  this  Pointer to the Linked List instance
     * @param[in]  func  Pointer to function that should be called for each node
     * @return LIST_ERR_OK on success, LIST_ERR_NULL_ARGUMENT or LIST_ERR_MULTITHREAD_ISSUE otherwise
     * @note This function returns if at any point func() does not return LIST_ERR_OK
     */
    ListError_t (*traverse)(struct List* this, ListError_t (*func)(void* data));

    /**
     * @brief Destroy this instance of List_t and free used resources
     * @param[in]  this  Pointer to the pointer to the Linked List instance
     * @return LIST_ERR_OK on success, LIST_ERR_MULTITHREAD_ISSUE or LIST_ERR_NULL_ARGUMENT otherwise
     */
    ListError_t (*destroy)(struct List** this);
} List_t;

/**
 * @brief Create a new linked list
 *
 * Allocates memory for a new linked list and initializes its mutex.
 *
 * @param[in] compare Function pointer for comparing node data.
 * @return Pointer to the new List_t instance, or NULL if memory allocation fails.
 */
List_t* llist_create(int (*compare)(const void* a, const void* b));

#endif // __LIST_H__