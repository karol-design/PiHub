#ifndef __PTHREAD_MOCKS_H__
#define __PTHREAD_MOCKS_H__

#include <pthread.h>

int __wrap_pthread_mutex_init(pthread_mutex_t* mutex);
int __wrap_pthread_mutex_lock(pthread_mutex_t* mutex);
int __wrap_pthread_mutex_unlock(pthread_mutex_t* mutex);
int __wrap_pthread_mutex_destroy(pthread_mutex_t* mutex);
int __wrap_pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg);
int __wrap_pthread_detach(pthread_t thread);
void __wrap_pthread_exit(void* retval);

#endif // __PTHREAD_MOCKS_H__