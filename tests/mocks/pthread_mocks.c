#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
// Cmocka must be included last (!)
#include <cmocka.h>

// Mocked library
#include "pthread_mocks.h"

/**
 * @brief Mock function for pthread_mutex_init() from the pthread.h library
 */
int __wrap_pthread_mutex_init(pthread_mutex_t* mutex) {
    return (int)mock();
}

/**
 * @brief Mock function for pthread_mutex_lock()
 */
int __wrap_pthread_mutex_lock(pthread_mutex_t* mutex) {
    return (int)mock();
}

/**
 * @brief Mock function for pthread_mutex_unlock()
 */
int __wrap_pthread_mutex_unlock(pthread_mutex_t* mutex) {
    return (int)mock();
}

/**
 * @brief Mock function for pthread_mutex_destroy()
 */
int __wrap_pthread_mutex_destroy(pthread_mutex_t* mutex) {
    return (int)mock();
}

/**
 * @brief Mock function for pthread_create()
 */
int __wrap_pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg) {
    return (int)mock();
}

/**
 * @brief Mock function for pthread_detach()
 */
int __wrap_pthread_detach(pthread_t thread) {
    return (int)mock();
}

/**
 * @brief Mock function for pthread_exit()
 */
void __wrap_pthread_exit(void* retval) {
}
