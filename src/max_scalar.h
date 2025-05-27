#ifndef __MAX_SCALAR_H
#define __MAX_SCALAR_H

#ifdef __SSE2__
#include <emmintrin.h>
#include <stdint.h>

typedef __m128i max_scalar_t;

#define MAX_SCALAR_LOAD(ptr)        _mm_loadu_si128((__m128i*)(ptr))
#define MAX_SCALAR_XOR(a, b)        _mm_xor_si128((a), (b))
#define MAX_SCALAR_STORE(ptr, val)  _mm_storeu_si128((__m128i*)(ptr), (val))

#else

#if __WORDSIZE == 64
typedef uint64_t max_scalar_t;
#else
typedef uint32_t max_scalar_t;
#endif // __WORDSIZE == 64

#define MAX_SCALAR_LOAD(ptr)        (*(max_scalar_t*)(ptr))
#define MAX_SCALAR_XOR(a, b)        ((a) ^ (b))
#define MAX_SCALAR_STORE(ptr, val)  (*(max_scalar_t*)(ptr) = (val))

#endif // __SSE2__

#endif // __MAX_SCALAR_H
