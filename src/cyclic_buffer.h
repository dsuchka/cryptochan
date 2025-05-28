#ifndef __CYCLIC_BUFFER_H
#define __CYCLIC_BUFFER_H

#ifndef CYCLIC_BUFFER_CHUNK_SIZE
# define CYCLIC_BUFFER_CHUNK_SIZE 0x1000
#endif

#ifndef CYCLIC_BUFFER_MAX_SIZE
# define CYCLIC_BUFFER_MAX_SIZE 0x40000000
#endif

#if (CYCLIC_BUFFER_CHUNK_SIZE <= 0) || ((CYCLIC_BUFFER_CHUNK_SIZE & 0xFFF) != 0)
# error "CYCLIC_BUFFER_CHUNK_SIZE is not a positive integer multiple of 4096 (4 KiB)"
#endif


typedef struct __cyclic_buffer {
    uint8_t* data_ptr;
    uint32_t total_size;
    uint32_t read_idx;
    uint32_t write_idx;
    uint32_t recode_idx;
    _Atomic uint32_t available_to_read;
    _Atomic uint32_t available_to_write;
    _Atomic uint32_t available_to_recode;
} cyclic_buffer_t;

extern bool cyclic_buffer_init(cyclic_buffer_t *buf, int chunks);
extern void cyclic_buffer_destroy(cyclic_buffer_t *buf);
extern uint32_t cyclic_buffer_read(cyclic_buffer_t *buf, uint8_t *dest, uint32_t max);
extern uint32_t cyclic_buffer_write(cyclic_buffer_t *buf, uint8_t *src, uint32_t max);
extern uint32_t cyclic_buffer_recode_none(cyclic_buffer_t *buf);
extern uint32_t cyclic_buffer_recode_xor(cyclic_buffer_t *dest, uint8_t *mask, uint32_t max);
extern uint32_t cyclic_buffer_recode_xor_buf(cyclic_buffer_t *buf, cyclic_buffer_t *mask_buf);

#endif // __CYCLIC_BUFFER_H
