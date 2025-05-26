#include "common.h"
#include "cyclic-buffer.h"

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

    return true;
}

void cyclic_buffer_destroy(cyclic_buffer_t *buf)
{
    if (buf->data_ptr) { free(buf->data_ptr); }

    memset(buf, 0, sizeof(cyclic_buffer_t));
}

void __cyclic_buffer_read_internal(cyclic_buffer_t *buf, uint8_t *dest, uint32_t size)
{
    // read data
    memcpy(dest, buf->data_ptr + buf->read_idx, size);

    // advance
    buf->available_size -= size;
    buf->read_idx += size;

    // restart read_idx when end is reached
    if (buf->read_idx >= buf->total_size) { buf->read_idx = 0; }
}

void __cyclic_buffer_write_internal(cyclic_buffer_t *buf, uint8_t *src, uint32_t size)
{
    // write data
    memcpy(buf->data_ptr + buf->write_idx, src, size);

    // advance
    buf->available_size += size;
    buf->write_idx += size;

    // restart write_idx when end is reached
    if (buf->write_idx >= buf->total_size) { buf->write_idx = 0; }
}

size_t cyclic_buffer_read(cyclic_buffer_t *buf, uint8_t *dest, size_t max)
{
    // do nothing when buffer is empty
    if (buf->available_size == 0) { return 0; }

    // read till end
    size_t size_till_end = (size_t)MIN(buf->total_size - buf->read_idx, buf->available_size);
    size_t to_read = MIN(size_till_end, max);
    __cyclic_buffer_read_internal(buf, dest, to_read);

    // finish when dest is full
    if (to_read == max) { return max; }

    // read remaining (if any)
    to_read = MIN((size_t)buf->available_size, max - size_till_end);
    if (to_read > 0) {
        __cyclic_buffer_read_internal(buf, dest + size_till_end, to_read);
    }

    // return total read size
    return size_till_end + to_read;
}

size_t cyclic_buffer_write(cyclic_buffer_t *buf, uint8_t *src, size_t max)
{
    // do nothing when buffer is full
    size_t w_size = (size_t)(buf->total_size - buf->available_size);
    if (w_size == 0) { return 0; }

    // write till end
    size_t size_till_end = (size_t)MIN(buf->total_size - buf->write_idx, w_size);
    size_t to_write = MIN(size_till_end, max);
    __cyclic_buffer_write_internal(buf, src, to_write);

    // finish when src is empty
    if (to_write == max) { return max; }

    // fix w_size for the written size
    w_size -= (uint32_t)to_write;

    // write remaining (if any)
    to_write = MIN(w_size, max - size_till_end);
    if (to_write > 0) {
        __cyclic_buffer_write_internal(buf, src + size_till_end, to_write);
    }

    // return total written size
    return size_till_end + to_write;
}

