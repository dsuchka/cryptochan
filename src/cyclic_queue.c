#include "common.h"
#include "cyclic_queue.h"

#include <sched.h>

#define __LOCK_IDX(idx_ptr, val_ptr) \
    do { \
        for(;;) { \
            *val_ptr = atomic_load_explicit(idx_ptr, memory_order_relaxed); \
            if (*val_ptr == UINT32_MAX) { sched_yield(); continue; } \
            break; \
        } \
    } while (!atomic_compare_exchange_weak_explicit( \
            idx_ptr, val_ptr, UINT32_MAX, \
            memory_order_acquire, memory_order_relaxed \
    ))

#define __UNLOCK_IDX(idx_ptr, val) \
    atomic_store_explicit(idx_ptr, val, memory_order_release)


bool cyclic_queue_init(cyclic_queue_t *queue, uint32_t element_size, uint32_t initial_capacity)
{
    memset(queue, 0, sizeof(cyclic_queue_t));

    if (initial_capacity < CYCLIC_QUEUE_MIN_CAPACITY) {
        initial_capacity = CYCLIC_QUEUE_MIN_CAPACITY;
    }

    if (initial_capacity > CYCLIC_QUEUE_MAX_CAPACITY) {
        fprintf(stderr, "ERROR: cyclic_queue_init: exceeded max capacity: %d (max: %d)\n",
            initial_capacity, CYCLIC_QUEUE_MAX_CAPACITY);
        return false;
    }

    if (element_size > CYCLIC_QUEUE_MAX_ELEMENT_SIZE) {
        fprintf(stderr, "ERROR: cyclic_queue_init: exceeded element size: %d (max: %d)\n",
            element_size, CYCLIC_QUEUE_MAX_ELEMENT_SIZE);
        return false;
    }

    if (!(queue->data_ptr = malloc(initial_capacity * element_size))) {
        perror("ERROR: cyclic_queue_init: malloc");
        return false;
    }

    queue->element_size = element_size;
    queue->capacity = initial_capacity;
    queue->max_capacity = CYCLIC_QUEUE_MAX_CAPACITY;

    return true;
}

void cyclic_queue_destroy(cyclic_queue_t *queue)
{
    if (queue->data_ptr) { free(queue->data_ptr); }

    memset(queue, 0, sizeof(cyclic_queue_t));
}


bool cyclic_queue_push(cyclic_queue_t *queue, void *element)
{
    const uint32_t elem_size = queue->element_size;
    uint32_t push_idx, take_idx = UINT32_MAX, size;
    bool result = false;

    // lock queue for push op
    __LOCK_IDX(&(queue->push_idx), &push_idx);

    do {
        // check queue size (is queue full?)
        size = atomic_load_explicit(&(queue->size), memory_order_relaxed);
        if (size == queue->capacity) {

            // queue is full! lock take op and recheck queue size
            __LOCK_IDX(&(queue->take_idx), &take_idx);
            size = atomic_load_explicit(&(queue->size), memory_order_acquire);
            if (size == queue->capacity) {
                uint32_t new_capacity = MIN(queue->capacity * 2, queue->max_capacity);

                // is max capacity reached? do exit with unsuccess status
                if (new_capacity == queue->capacity) {
                    break;
                }

                // do custom re-alloc (alloc new region to move data into)
                void *new_data_ptr = malloc(elem_size * new_capacity);
                if (!new_data_ptr) {
                    fprintf(stderr, "ERROR: cyclic_queue_push: malloc: %s\n",
                        strerror(errno));
                    break;
                }

                // move data to the new allocated region
                uint32_t until_idx = take_idx + size;
                if (until_idx > queue->capacity) {
                    uint32_t size_till_end = queue->capacity - take_idx;
                    memcpy(new_data_ptr,
                        (uint8_t*)queue->data_ptr + take_idx * elem_size,
                        size_till_end * elem_size);
                    memcpy((uint8_t*)new_data_ptr + size_till_end * elem_size,
                        queue->data_ptr,
                        (size - size_till_end) * elem_size);
                } else {
                    memcpy(new_data_ptr,
                        (uint8_t*)queue->data_ptr + take_idx * elem_size,
                        size * elem_size);
                }

                // free previsouly allocated regoin and set new one
                free(queue->data_ptr);
                queue->data_ptr = new_data_ptr;

                // update capacity
                queue->capacity = new_capacity;

                // update take idx: now it points to the beginning
                take_idx = 0; // will be set later on __UNLOCK_IDX

                // update push idx: now it points to the end
                push_idx = size; // will be set later on __UNLOCK_IDX
            }
        }

        // push next element
        memcpy((uint8_t*)queue->data_ptr + push_idx * elem_size, element, elem_size);
        push_idx = (push_idx + 1) % queue->capacity;

        // update size
        atomic_fetch_add_explicit(&(queue->size), 1, memory_order_release);

        // all done
        result = true;
    } while (false);

    // unlock idx vars
    if (take_idx != UINT32_MAX) {
        __UNLOCK_IDX(&(queue->take_idx), take_idx);
    }
    __UNLOCK_IDX(&(queue->push_idx), push_idx);

    // return result
    return result;
}

bool cyclic_queue_take(cyclic_queue_t *queue, void *element)
{
    // check queue size (is queue empty?)
    if (!atomic_load_explicit(&(queue->size), memory_order_relaxed))
        { return false; }

    const uint32_t elem_size = queue->element_size;
    uint32_t take_idx;
    bool result = false;

    // lock queue for take op
    __LOCK_IDX(&(queue->take_idx), &take_idx);

    do {
        // recheck queue size (after lock is acquired)
        if (!atomic_load_explicit(&(queue->size), memory_order_relaxed))
            { break; }

        // take next element
        memcpy(element, (uint8_t*)queue->data_ptr + take_idx * elem_size, elem_size);
        take_idx = (take_idx + 1) % queue->capacity;

        // update size
        atomic_fetch_sub_explicit(&(queue->size), 1, memory_order_relaxed);

        // all done
        result = true;
    } while (false);

    // unlock queue for take op
    __UNLOCK_IDX(&(queue->take_idx), take_idx);

    // return result
    return result;
}

