#ifndef __MAX_SCALAR_H
#define __MAX_SCALAR_H

#if defined(__AVX2__)

# include <immintrin.h>

typedef __m256i max_scalar_t;

# define MAX_SCALAR_LOAD(ptr)           _mm256_loadu_si256((__m256i*)(ptr))
# define MAX_SCALAR_XOR(a, b)           _mm256_xor_si256((a), (b))
# define MAX_SCALAR_STORE(ptr, val)     _mm256_storeu_si256((__m256i*)(ptr), (val))

#elif defined(__SSE2__)

# include <emmintrin.h>

typedef __m128i max_scalar_t;

# define MAX_SCALAR_LOAD(ptr)           _mm_loadu_si128((__m128i*)(ptr))
# define MAX_SCALAR_XOR(a, b)           _mm_xor_si128((a), (b))
# define MAX_SCALAR_STORE(ptr, val)     _mm_storeu_si128((__m128i*)(ptr), (val))

#else // uint

# include <stdint.h>

# if __WORDSIZE == 64
typedef uint64_t max_scalar_t;
# else
typedef uint32_t max_scalar_t;
# endif // __WORDSIZE == 64

# define MAX_SCALAR_LOAD(ptr)           (*(max_scalar_t*)(ptr))
# define MAX_SCALAR_XOR(a, b)           ((a) ^ (b))
# define MAX_SCALAR_STORE(ptr, val)     (*(max_scalar_t*)(ptr) = (val))

#endif // __AVX2__/__SSE2__/uint

#endif // __MAX_SCALAR_H
