#include "common.h"
#include "cyclic-buffer.h"
#include "max-scalar.h"

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

void __cyclic_buffer_read_internal(cyclic_buffer_t *buf, uint8_t *dest, uint32_t size)
{
    // read data
    memcpy(dest, buf->data_ptr + buf->read_idx, size);

    // advance
    buf->available_to_read -= size;
    buf->available_to_write += size;
    buf->read_idx += size;

    // restart read_idx when end is reached
    if (buf->read_idx >= buf->total_size) { buf->read_idx = 0; }
}

void __cyclic_buffer_write_internal(cyclic_buffer_t *buf, uint8_t *src, uint32_t size)
{
    // write data
    memcpy(buf->data_ptr + buf->write_idx, src, size);

    // advance
    buf->available_to_write -= size;
    buf->available_to_recode += size;
    buf->write_idx += size;

    // restart write_idx when end is reached
    if (buf->write_idx >= buf->total_size) { buf->write_idx = 0; }
}

uint32_t cyclic_buffer_read(cyclic_buffer_t *buf, uint8_t *dest, uint32_t max)
{
    // do nothing when buffer is empty
    if (buf->available_to_read == 0) { return 0; }

    // read till end
    uint32_t size_till_end = MIN(buf->total_size - buf->read_idx, buf->available_to_read);
    uint32_t to_read = MIN(size_till_end, max);
    __cyclic_buffer_read_internal(buf, dest, to_read);

    // finish when dest is full
    if (to_read == max) { return max; }

    // read remaining (if any)
    to_read = MIN(buf->available_to_read, max - size_till_end);
    if (to_read > 0) {
        __cyclic_buffer_read_internal(buf, dest + size_till_end, to_read);
    }

    // return total read size
    return size_till_end + to_read;
}

uint32_t cyclic_buffer_write(cyclic_buffer_t *buf, uint8_t *src, uint32_t max)
{
    // do nothing when buffer is full
    if (buf->available_to_write == 0) { return 0; }

    // write till end
    uint32_t size_till_end = MIN(buf->total_size - buf->write_idx, buf->available_to_write);
    uint32_t to_write = MIN(size_till_end, max);
    __cyclic_buffer_write_internal(buf, src, to_write);

    // finish when src is empty
    if (to_write == max) { return max; }

    // write remaining (if any)
    to_write = MIN(buf->available_to_write, max - size_till_end);
    if (to_write > 0) {
        __cyclic_buffer_write_internal(buf, src + size_till_end, to_write);
    }

    // return total written size
    return size_till_end + to_write;
}

uint32_t cyclic_buffer_recode_none(cyclic_buffer_t *buf)
{
    uint32_t size = buf->available_to_recode;
    buf->available_to_read += size;
    buf->available_to_recode = 0;
    return size;
}

void __cyclic_buffer_recode_xor_internal(uint8_t *dptr, uint8_t *mask, uint32_t size)
{
    // unroll loop for 4 x max scalars
    while (size >= sizeof(max_scalar_t) * 4) {
        register max_scalar_t s0 = MAX_SCALAR_LOAD(dptr + sizeof(max_scalar_t) * 0);
        register max_scalar_t s1 = MAX_SCALAR_LOAD(dptr + sizeof(max_scalar_t) * 1);
        register max_scalar_t s2 = MAX_SCALAR_LOAD(dptr + sizeof(max_scalar_t) * 2);
        register max_scalar_t s3 = MAX_SCALAR_LOAD(dptr + sizeof(max_scalar_t) * 3);

        register max_scalar_t m0 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 0);
        register max_scalar_t m1 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 1);
        register max_scalar_t m2 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 2);
        register max_scalar_t m3 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 3);

        MAX_SCALAR_STORE(dptr + sizeof(max_scalar_t) * 0, MAX_SCALAR_XOR(s0, m0));
        MAX_SCALAR_STORE(dptr + sizeof(max_scalar_t) * 1, MAX_SCALAR_XOR(s1, m1));
        MAX_SCALAR_STORE(dptr + sizeof(max_scalar_t) * 2, MAX_SCALAR_XOR(s2, m2));
        MAX_SCALAR_STORE(dptr + sizeof(max_scalar_t) * 3, MAX_SCALAR_XOR(s3, m3));

        size -= sizeof(max_scalar_t) * 4;
        dptr += sizeof(max_scalar_t) * 4;
        mask += sizeof(max_scalar_t) * 4;
    }

    // process remaining max scalars
    while (size >= sizeof(max_scalar_t)) {
        max_scalar_t s0 = MAX_SCALAR_LOAD(dptr + sizeof(max_scalar_t) * 0);
        max_scalar_t m0 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 0);
        MAX_SCALAR_STORE(dptr + sizeof(max_scalar_t) * 0, MAX_SCALAR_XOR(s0, m0));
        size -= sizeof(max_scalar_t);
        dptr += sizeof(max_scalar_t);
        mask += sizeof(max_scalar_t);
    }

    // process remaining bytes
    while (size-- > 0) {
        *(dptr++) ^= *(mask++);
    }
}


uint32_t cyclic_buffer_recode_xor(cyclic_buffer_t *buf, uint8_t *mask, uint32_t size)
{
    // do nothing when buffer has no data to be recoded
    if (buf->available_to_recode == 0) { return 0; }

    uint8_t *end_ptr = buf->data_ptr + buf->total_size;
    uint8_t *dptr = buf->data_ptr + buf->write_idx - buf->available_to_recode;
    uint32_t to_recode = MIN(buf->available_to_recode, size);
    size = to_recode; // return value

    // fix dptr when out of bounds and process tail data
    if (dptr < buf->data_ptr) {
        dptr += buf->total_size;
        uint32_t tail_size = MIN(end_ptr - dptr, to_recode);

        __cyclic_buffer_recode_xor_internal(dptr, mask, tail_size);

        // rewind dptr and mask, consume recoded size
        dptr = buf->data_ptr;
        to_recode -= tail_size;
        mask += tail_size;
    }

    if (to_recode) {
        __cyclic_buffer_recode_xor_internal(dptr, mask, to_recode);
    }

    // advance
    buf->available_to_read += size;
    buf->available_to_recode -= size;

    return size;
}

