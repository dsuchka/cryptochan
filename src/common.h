#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <unistd.h>
#include <sys/param.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

extern void assure_error_desc_empty(char **error_desc);
extern void xor_memory_region(uint8_t *dptr, uint8_t *mask, size_t size);

#endif // __COMMON_H
