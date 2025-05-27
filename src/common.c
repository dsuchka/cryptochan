#include "common.h"
#include "max_scalar.h"

void assure_error_desc_empty(char **error_desc)
{
    // assure that error_desc does not hold any valid ptr
    if (*error_desc != NULL) {
        fprintf(stderr, "PANIC: error_desc ptr = %p (expected NULL)\n", (void*) error_desc);
        abort();
    }
}

void __xor_memory_region_simple(uint8_t *dptr, uint8_t *mask, size_t size)
{
#if (MAX_SCALAR_SIZE >= 64)
    // process remaining quad words
    while (size > sizeof(uint64_t)) {
        register uint64_t v0 = *((uint64_t*)dptr);
        register uint64_t m0 = *((uint64_t*)mask);

        *((uint64_t*)dptr) = v0 ^ m0;

        size -= sizeof(uint64_t);
        dptr += sizeof(uint64_t);
        mask += sizeof(uint64_t);
    }
#endif

    // process remaining double words
#if (MAX_SCALAR_SIZE >= 64)
    if
#else
    while
#endif
    (size > sizeof(uint32_t)) {
        register uint32_t v0 = *((uint32_t*)dptr);
        register uint32_t m0 = *((uint32_t*)mask);

        *((uint32_t*)dptr) = v0 ^ m0;

        size -= sizeof(uint32_t);
        dptr += sizeof(uint32_t);
        mask += sizeof(uint32_t);
    }

    // process remaining bytes
    while (size-- > 0) {
        *(dptr++) ^= *(mask++);
    }

    return;
}

#if (MAX_SCALAR_SIZE > 64)

void __xor_memory_region_aligned_both(uint8_t *dptr, uint8_t *mask, size_t size)
{
    // unroll loop for 4 x max scalars
    while (size >= sizeof(max_scalar_t) * 4) {
        register max_scalar_t s0 = MAX_SCALAR_LOAD_ALIGNED(dptr + sizeof(max_scalar_t) * 0);
        register max_scalar_t s1 = MAX_SCALAR_LOAD_ALIGNED(dptr + sizeof(max_scalar_t) * 1);
        register max_scalar_t m0 = MAX_SCALAR_LOAD_ALIGNED(mask + sizeof(max_scalar_t) * 0);
        register max_scalar_t m1 = MAX_SCALAR_LOAD_ALIGNED(mask + sizeof(max_scalar_t) * 1);

        MAX_SCALAR_STORE_ALIGNED(dptr + sizeof(max_scalar_t) * 0, MAX_SCALAR_XOR(s0, m0));
        MAX_SCALAR_STORE_ALIGNED(dptr + sizeof(max_scalar_t) * 1, MAX_SCALAR_XOR(s1, m1));

        register max_scalar_t s2 = MAX_SCALAR_LOAD_ALIGNED(dptr + sizeof(max_scalar_t) * 2);
        register max_scalar_t s3 = MAX_SCALAR_LOAD_ALIGNED(dptr + sizeof(max_scalar_t) * 3);
        register max_scalar_t m2 = MAX_SCALAR_LOAD_ALIGNED(mask + sizeof(max_scalar_t) * 2);
        register max_scalar_t m3 = MAX_SCALAR_LOAD_ALIGNED(mask + sizeof(max_scalar_t) * 3);

        MAX_SCALAR_STORE_ALIGNED(dptr + sizeof(max_scalar_t) * 2, MAX_SCALAR_XOR(s2, m2));
        MAX_SCALAR_STORE_ALIGNED(dptr + sizeof(max_scalar_t) * 3, MAX_SCALAR_XOR(s3, m3));

        size -= sizeof(max_scalar_t) * 4;
        dptr += sizeof(max_scalar_t) * 4;
        mask += sizeof(max_scalar_t) * 4;
    }

    // process remaining max scalars
    while (size >= sizeof(max_scalar_t)) {
        register max_scalar_t s0 = MAX_SCALAR_LOAD_ALIGNED(dptr + sizeof(max_scalar_t) * 0);
        register max_scalar_t m0 = MAX_SCALAR_LOAD_ALIGNED(mask + sizeof(max_scalar_t) * 0);

        MAX_SCALAR_STORE_ALIGNED(dptr + sizeof(max_scalar_t) * 0, MAX_SCALAR_XOR(s0, m0));

        size -= sizeof(max_scalar_t);
        dptr += sizeof(max_scalar_t);
        mask += sizeof(max_scalar_t);
    }

    // process remaining simply
    if (size) {
        __xor_memory_region_simple(dptr, mask, size);
    }

    return;
}

void __xor_memory_region_aligned_dptr(uint8_t *dptr, uint8_t *mask, size_t size)
{
    // unroll loop for 4 x max scalars
    while (size >= sizeof(max_scalar_t) * 4) {
        register max_scalar_t s0 = MAX_SCALAR_LOAD_ALIGNED(dptr + sizeof(max_scalar_t) * 0);
        register max_scalar_t s1 = MAX_SCALAR_LOAD_ALIGNED(dptr + sizeof(max_scalar_t) * 1);
        register max_scalar_t m0 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 0);
        register max_scalar_t m1 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 1);

        MAX_SCALAR_STORE_ALIGNED(dptr + sizeof(max_scalar_t) * 0, MAX_SCALAR_XOR(s0, m0));
        MAX_SCALAR_STORE_ALIGNED(dptr + sizeof(max_scalar_t) * 1, MAX_SCALAR_XOR(s1, m1));

        register max_scalar_t s2 = MAX_SCALAR_LOAD_ALIGNED(dptr + sizeof(max_scalar_t) * 2);
        register max_scalar_t s3 = MAX_SCALAR_LOAD_ALIGNED(dptr + sizeof(max_scalar_t) * 3);
        register max_scalar_t m2 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 2);
        register max_scalar_t m3 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 3);

        MAX_SCALAR_STORE_ALIGNED(dptr + sizeof(max_scalar_t) * 2, MAX_SCALAR_XOR(s2, m2));
        MAX_SCALAR_STORE_ALIGNED(dptr + sizeof(max_scalar_t) * 3, MAX_SCALAR_XOR(s3, m3));

        size -= sizeof(max_scalar_t) * 4;
        dptr += sizeof(max_scalar_t) * 4;
        mask += sizeof(max_scalar_t) * 4;
    }

    // process remaining max scalars
    while (size >= sizeof(max_scalar_t)) {
        register max_scalar_t s0 = MAX_SCALAR_LOAD_ALIGNED(dptr + sizeof(max_scalar_t) * 0);
        register max_scalar_t m0 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 0);

        MAX_SCALAR_STORE_ALIGNED(dptr + sizeof(max_scalar_t) * 0, MAX_SCALAR_XOR(s0, m0));

        size -= sizeof(max_scalar_t);
        dptr += sizeof(max_scalar_t);
        mask += sizeof(max_scalar_t);
    }

    // process remaining simply
    if (size) {
        __xor_memory_region_simple(dptr, mask, size);
    }

    return;
}

#endif // (MAX_SCALAR_SIZE > 64)


void xor_memory_region(uint8_t *dptr, uint8_t *mask, size_t size)
{
    size_t dptr_unaligned_tail = (size_t)dptr & (~MAX_SCALAR_ALIGN_MASK);

    // do align (process) if there some portion of unaligned bytes
    if (dptr_unaligned_tail) {
        size_t count = MAX_SCALAR_BYTES - dptr_unaligned_tail;

        if (size <= count) {
            __xor_memory_region_simple(dptr, mask, size);

            return;
        }

        __xor_memory_region_simple(dptr, mask, count);

        size -= count;
        dptr += count;
        mask += count;
    }

#if (MAX_SCALAR_SIZE > 64)
    size_t mask_unaligned_tail = (size_t)mask & (~MAX_SCALAR_ALIGN_MASK);

    if (mask_unaligned_tail) {
        __xor_memory_region_aligned_dptr(dptr, mask, size);
    } else {
        __xor_memory_region_aligned_both(dptr, mask, size);
    }
#else
    __xor_memory_region_simple(dptr, mask, size);
#endif

    return;
}

