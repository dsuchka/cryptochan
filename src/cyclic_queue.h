#ifndef __CYCLIC_QUEUE_H
#define __CYCLIC_QUEUE_H

#ifndef CYCLIC_QUEUE_MIN_CAPACITY
# define CYCLIC_QUEUE_MIN_CAPACITY 0x40
#endif

#ifndef CYCLIC_QUEUE_MAX_CAPACITY
# define CYCLIC_QUEUE_MAX_CAPACITY 0x40000
#endif

#ifndef CYCLIC_QUEUE_MAX_ELEMENT_SIZE
# define CYCLIC_QUEUE_MAX_ELEMENT_SIZE 0x400
#endif

typedef struct __cyclic_queue {
    void *data_ptr;
    uint32_t element_size;
    uint32_t capacity;
    uint32_t max_capacity;
    _Atomic uint32_t size;
    _Atomic uint32_t take_idx;
    _Atomic uint32_t push_idx;
} cyclic_queue_t;

extern bool cyclic_queue_init(cyclic_queue_t *queue, uint32_t element_size, uint32_t initial_capacity);
extern void cyclic_queue_destroy(cyclic_queue_t *queue);
extern bool cyclic_queue_push(cyclic_queue_t *queue, void *element);
extern bool cyclic_queue_take(cyclic_queue_t *queue, void *element);

#endif // __CYCLIC_QUEUE_H
