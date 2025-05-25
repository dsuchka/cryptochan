#ifndef __RANDOM_H
#define __RANDOM_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

extern void set_use_prng(bool use);
extern ssize_t fill_random_next(uint8_t *buf, size_t max);
extern ssize_t fill_random(uint8_t *buf, size_t max);

#endif // __RANDOM_H
