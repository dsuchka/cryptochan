#include "random.h"

#include <unistd.h>
#include <sys/random.h>
#include <sys/time.h>

bool do_use_prng = false;

void set_use_prng(bool use) {
    do_use_prng = use;
}

uint32_t xoshiro128plus(uint32_t *state) {
    uint32_t result = state[0] + state[3];

    uint32_t t = state[1] << 9;

    state[2] ^= state[0];
    state[3] ^= state[1];
    state[1] ^= state[2];
    state[0] ^= state[3];

    state[2] ^= t;

    state[3] = (state[3] << 11) | (state[3] >> (32 - 11));

    return result;
}

#include <stdio.h>

ssize_t fill_prng_random(uint8_t *buf, size_t max) {
    uint32_t state[4];
    struct timeval tm;

    // fill state using time and pid as a seed
    gettimeofday(&tm, NULL);
    uint32_t seed = ((uint32_t)tm.tv_sec + (uint32_t)getpid()) ^ (uint32_t)tm.tv_usec;
    for (int i = 0; i < 4; i++) {
        seed = state[i] = (seed * 0x1b1 + i);
    }

    // fill data
    ssize_t filled = 0;
    size_t max_ = (max / sizeof(uint32_t)) * sizeof(uint32_t);
    for (; filled < max_; filled += sizeof(uint32_t)) {
        uint32_t next = xoshiro128plus(state);
        *((uint32_t*)(buf + filled)) = next;
    }
    if (filled < max) {
        uint32_t next = xoshiro128plus(state);
        for (; filled < max; ++filled) {
            *(buf+filled) = (uint8_t)next;
            next >>= 8;
        }
    }

    return filled;
}

ssize_t fill_random_next(uint8_t *buf, size_t max) {
    return do_use_prng
        ? fill_prng_random(buf, max)
        : getrandom(buf, max, 0);
}

ssize_t fill_random(uint8_t *buf, size_t max) {
    ssize_t filled = 0;
    while (filled < max) {
        ssize_t n = fill_random_next(buf, max - filled);
        if (n < 0) {
            perror("failed to fill random data");
            return n;
        }
        filled += n;
    }
    return filled;
}

