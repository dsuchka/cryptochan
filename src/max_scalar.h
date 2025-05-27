#ifndef __MAX_SCALAR_H
#define __MAX_SCALAR_H


//
//  AVX2
//

#if defined(__AVX2__)

# define MAX_SCALAR_SIZE                        256

# include <immintrin.h>

typedef __m256i scalar256_t;

# define SCALAR256_LOAD(ptr)                    _mm256_loadu_si256((__m256i*)(ptr))
# define SCALAR256_LOAD_ALIGNED(ptr)            _mm256_load_si256((__m256i*)(ptr))
# define SCALAR256_STORE(ptr, val)              _mm256_storeu_si256((__m256i*)(ptr), (val))
# define SCALAR256_STORE_ALIGNED(ptr, val)      _mm256_store_si256((__m256i*)(ptr), (val))
# define SCALAR256_XOR(a, b)                    _mm256_xor_si256((a), (b))

#endif // __AVX2__


//
//  SSE2
//

#if defined(__SSE2__)

# ifndef MAX_SCALAR_SIZE
#  define MAX_SCALAR_SIZE                       128
# endif // MAX_SCALAR_SIZE

# include <emmintrin.h>

typedef __m128i scalar128_t;

# define SCALAR128_LOAD(ptr)                    _mm_loadu_si128((__m128i*)(ptr))
# define SCALAR128_LOAD_ALIGNED(ptr)            _mm_load_si128((__m128i*)(ptr))
# define SCALAR128_STORE(ptr, val)              _mm_storeu_si128((__m128i*)(ptr), (val))
# define SCALAR128_STORE_ALIGNED(ptr, val)      _mm_storeu_si128((__m128i*)(ptr), (val))
# define SCALAR128_XOR(a, b)                    _mm_xor_si128((a), (b))

#endif // __SSE2__


//
//  MAX_SCALAR
//

#if (MAX_SCALAR_SIZE == 256)

typedef __m256i max_scalar_t;

# define MAX_SCALAR_LOAD(ptr)                   SCALAR256_LOAD(ptr)
# define MAX_SCALAR_LOAD_ALIGNED(ptr)           SCALAR256_LOAD_ALIGNED(ptr)
# define MAX_SCALAR_STORE(ptr, val)             SCALAR256_STORE(ptr, val)
# define MAX_SCALAR_STORE_ALIGNED(ptr, val)     SCALAR256_STORE_ALIGNED(ptr, val)
# define MAX_SCALAR_XOR(a, b)                   SCALAR256_XOR(a, b)

#elif (MAX_SCALAR_SIZE == 128)

typedef __m128i max_scalar_t;

# define MAX_SCALAR_LOAD(ptr)                   SCALAR128_LOAD(ptr)
# define MAX_SCALAR_LOAD_ALIGNED(ptr)           SCALAR128_LOAD_ALIGNED(ptr)
# define MAX_SCALAR_STORE(ptr, val)             SCALAR128_STORE(ptr, val)
# define MAX_SCALAR_STORE_ALIGNED(ptr, val)     SCALAR128_STORE_ALIGNED(ptr, val)
# define MAX_SCALAR_XOR(a, b)                   SCALAR128_XOR(a, b)

#else

# include <stdint.h>

# if (__WORDSIZE == 64)
#  define MAX_SCALAR_SIZE                       64
typedef uint64_t max_scalar_t;
# else
#  define MAX_SCALAR_SIZE                       32
typedef uint32_t max_scalar_t;
# endif // __WORDSIZE == 64

# define MAX_SCALAR_LOAD(ptr)                   (*(max_scalar_t*)(ptr))
# define MAX_SCALAR_XOR(a, b)                   ((a) ^ (b))
# define MAX_SCALAR_STORE(ptr, val)             (*(max_scalar_t*)(ptr) = (val))

#endif // MAX_SCALAR_SIZE 256/128/(64/32)

#define MAX_SCALAR_BYTES                        ((MAX_SCALAR_SIZE) >> 3)
#define MAX_SCALAR_ALIGN_MASK                   (~((MAX_SCALAR_BYTES) - 1))

#endif // __MAX_SCALAR_H
