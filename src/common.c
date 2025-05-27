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

void xor_memory_region(uint8_t *dptr, uint8_t *mask, size_t size)
{
    // unroll loop for 4 x max scalars
    while (size >= sizeof(max_scalar_t) * 4) {
        {
            register max_scalar_t s0 = MAX_SCALAR_LOAD(dptr + sizeof(max_scalar_t) * 0);
            register max_scalar_t s1 = MAX_SCALAR_LOAD(dptr + sizeof(max_scalar_t) * 1);

            register max_scalar_t m0 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 0);
            register max_scalar_t m1 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 1);

            MAX_SCALAR_STORE(dptr + sizeof(max_scalar_t) * 0, MAX_SCALAR_XOR(s0, m0));
            MAX_SCALAR_STORE(dptr + sizeof(max_scalar_t) * 1, MAX_SCALAR_XOR(s1, m1));
        }

        {
            register max_scalar_t s2 = MAX_SCALAR_LOAD(dptr + sizeof(max_scalar_t) * 2);
            register max_scalar_t s3 = MAX_SCALAR_LOAD(dptr + sizeof(max_scalar_t) * 3);

            register max_scalar_t m2 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 2);
            register max_scalar_t m3 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 3);

            MAX_SCALAR_STORE(dptr + sizeof(max_scalar_t) * 2, MAX_SCALAR_XOR(s2, m2));
            MAX_SCALAR_STORE(dptr + sizeof(max_scalar_t) * 3, MAX_SCALAR_XOR(s3, m3));
        }

        size -= sizeof(max_scalar_t) * 4;
        dptr += sizeof(max_scalar_t) * 4;
        mask += sizeof(max_scalar_t) * 4;
    }

    // process remaining max scalars
    while (size >= sizeof(max_scalar_t)) {
        register max_scalar_t s0 = MAX_SCALAR_LOAD(dptr + sizeof(max_scalar_t) * 0);
        register max_scalar_t m0 = MAX_SCALAR_LOAD(mask + sizeof(max_scalar_t) * 0);

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

