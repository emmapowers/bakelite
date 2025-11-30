#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

/* Packed struct attribute for zero-copy mode */
#if defined(__GNUC__) || defined(__clang__)
  #define BAKELITE_PACKED __attribute__((packed))
#elif defined(_MSC_VER)
  #define BAKELITE_PACKED
  #pragma pack(push, 1)
#else
  #define BAKELITE_PACKED
#endif

/* Static assert for compile-time checks (C99 compatible) */
#define BAKELITE_STATIC_ASSERT(cond, msg) \
    typedef char bakelite_static_assert_##msg[(cond) ? 1 : -1]

/* Platform detection for unaligned access support */
#if defined(__AVR__) || defined(__arm__) || defined(__aarch64__) || \
    defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
  #define BAKELITE_UNALIGNED_OK 1
#else
  #define BAKELITE_UNALIGNED_OK 0
#endif

/* Error codes */
#define BAKELITE_OK              0
#define BAKELITE_ERR_WRITE      -1
#define BAKELITE_ERR_READ       -2
#define BAKELITE_ERR_SEEK       -3
#define BAKELITE_ERR_ALLOC      -4
#define BAKELITE_ERR_ALLOC_BYTES -5
#define BAKELITE_ERR_ALLOC_STRING -6

/* Sized array for variable-length data */
typedef struct {
    void *data;
    uint8_t size;
} Bakelite_SizedArray;

/* Sized array with 16-bit size */
typedef struct {
    void *data;
    uint16_t size;
} Bakelite_SizedArray16;
