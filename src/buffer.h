#include <pthread.h>

#ifndef __BUFFER_H
#define __BUFFER_H


#ifndef BUFFER_CHUNK_SIZE
#define BUFFER_CHUNK_SIZE 4096
#endif

typedef struct __custom_buffer {
    const static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    const pthread_cond_t cond_can_read = PTHREAD_COND_INITIALIZER;
    const pthread_cond_t cond_can_write = PTHREAD_COND_INITIALIZER;
    void* buffer_data_ptr;
    const uint32_t buffer_size;
    volatile uint32_t seqnum;
    volatile uint32_t next_read_idx;
    volatile uint32_t next_write_idx;
} custom_buffer_t;

#endif // __BUFFER_H
