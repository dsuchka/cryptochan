#ifndef __RANDOM_H
#define __RANDOM_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

extern ssize_t fill_random(uint8_t *buf, size_t max);
extern void set_use_prng(bool use);

#endif // __RANDOM_H
