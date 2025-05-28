#include "common.h"
#include "cyclic_buffer.h"

#include <malloc.h>

bool cyclic_buffer_init(cyclic_buffer_t *buf, int chunks)
{
    memset(buf, 0, sizeof(cyclic_buffer_t));

    if (chunks <= 0) {
        fprintf(stderr, "ERROR: cyclic_buffer_init: negative chunks = %d\n", chunks);
        return false;
    }

    buf->total_size = CYCLIC_BUFFER_CHUNK_SIZE * chunks;

    if (buf->total_size > CYCLIC_BUFFER_MAX_SIZE) {
        fprintf(stderr, "ERROR: cyclic_buffer_init: exceeded max buffer size: %d (max: %d)\n",
            buf->total_size, CYCLIC_BUFFER_MAX_SIZE);
        return false;
    }

    if (!(buf->data_ptr = malloc(buf->total_size))) {
        perror("ERROR: cyclic_buffer_init: malloc");
        return false;
    }

    buf->available_to_write = buf->total_size;

    return true;
}

void cyclic_buffer_destroy(cyclic_buffer_t *buf)
{
    if (buf->data_ptr) { free(buf->data_ptr); }

    memset(buf, 0, sizeof(cyclic_buffer_t));
}

uint32_t cyclic_buffer_read(cyclic_buffer_t *buf, uint8_t *dest, uint32_t max)
{
    // get readable size (synchronized)
    uint32_t available_to_read = atomic_load_explicit(
        &(buf->available_to_read), memory_order_acquire);

    // do nothing when buffer is empty
    if (available_to_read == 0) { return 0; }

    // read not more than requested (max size)
    uint32_t size = MIN(available_to_read, max);

    // do read
    uint32_t next_read_idx = buf->read_idx + size;
    if (next_read_idx > buf->total_size) {
        uint32_t size_till_end = buf->total_size - buf->read_idx;
        next_read_idx = size - size_till_end;
        memcpy(dest, buf->data_ptr + buf->read_idx, size_till_end);
        memcpy(dest + size_till_end, buf->data_ptr, next_read_idx);
    } else {
        memcpy(dest, buf->data_ptr + buf->read_idx, size);
        next_read_idx %= buf->total_size;
    }

    // advance
    buf->read_idx = next_read_idx;
    atomic_fetch_add_explicit(&(buf->available_to_write), size, memory_order_relaxed);
    atomic_fetch_sub_explicit(&(buf->available_to_read), size, memory_order_relaxed);

    // return size of read data
    return size;
}

uint32_t cyclic_buffer_write(cyclic_buffer_t *buf, uint8_t *src, uint32_t max)
{
    // get writeable size (synchronized)
    uint32_t available_to_write = atomic_load_explicit(
        &(buf->available_to_write), memory_order_relaxed);

    // do nothing when buffer is full
    if (available_to_write == 0) { return 0; }

    // write not more than requested (max size)
    uint32_t size = MIN(available_to_write, max);

    // do write
    uint32_t next_write_idx = buf->write_idx + size;
    if (next_write_idx > buf->total_size) {
        uint32_t size_till_end = buf->total_size - buf->write_idx;
        next_write_idx = size - size_till_end;
        memcpy(buf->data_ptr + buf->write_idx, src, size_till_end);
        memcpy(buf->data_ptr, src + size_till_end, next_write_idx);
    } else {
        memcpy(buf->data_ptr + buf->write_idx, src, size);
        next_write_idx %= buf->total_size;
    }

    // advance
    buf->write_idx = next_write_idx;
    atomic_fetch_add_explicit(&(buf->available_to_recode), size, memory_order_release);
    atomic_fetch_sub_explicit(&(buf->available_to_write), size, memory_order_relaxed);

    // return size of written data
    return size;
}

uint32_t cyclic_buffer_recode_none(cyclic_buffer_t *buf)
{
    uint32_t size = atomic_load_explicit(
        &(buf->available_to_recode), memory_order_relaxed);
    buf->recode_idx = (buf->recode_idx + size) % buf->total_size;
    atomic_fetch_add_explicit(&(buf->available_to_read), size, memory_order_relaxed);
    atomic_store_explicit(&(buf->available_to_recode), 0, memory_order_relaxed);
    return size;
}

uint32_t cyclic_buffer_recode_xor(cyclic_buffer_t *buf, uint8_t *mask, uint32_t max)
{
    // get recodeable size (synchronized)
    uint32_t available_to_recode = atomic_load_explicit(
        &(buf->available_to_recode), memory_order_acquire);

    // do nothing when buffer has no data to be recoded
    if (available_to_recode == 0) { return 0; }

    // recode not more than requested (max size)
    uint32_t size = MIN(available_to_recode, max);

    // do recode (XOR with given mask)
    uint32_t next_recode_idx = buf->recode_idx + size;
    if (next_recode_idx > buf->total_size) {
        uint32_t size_till_end = buf->total_size - buf->recode_idx;
        next_recode_idx = size - size_till_end;
        xor_memory_region(buf->data_ptr + buf->recode_idx, mask, size_till_end);
        xor_memory_region(buf->data_ptr, mask + size_till_end, next_recode_idx);
    } else {
        xor_memory_region(buf->data_ptr + buf->recode_idx, mask, size);
        next_recode_idx %= buf->total_size;
    }

    // advance
    buf->recode_idx = next_recode_idx;
    atomic_fetch_add_explicit(&(buf->available_to_read), size, memory_order_release);
    atomic_fetch_sub_explicit(&(buf->available_to_recode), size, memory_order_relaxed);

    // return size of recoded data
    return size;
}

