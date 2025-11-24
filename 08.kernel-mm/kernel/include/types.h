#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

/*
 * kernel/types.h
 * Minimal type and utility header for a freestanding 32-bit i386 kernel.
 *
 * Notes:
 *  - Target: 32-bit i386 kernel. When building on a 64-bit host, compile with -m32
 *    or use a cross-compiler to ensure sizeof(uintptr_t)==4.
 *  - This header is freestanding (no dependency on the C standard library).
 *  - Keep macros and names conservative to reduce collisions.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- fixed-width integer aliases ---------------- */
typedef unsigned char      u8;
typedef signed char        s8;
typedef unsigned short     u16;
typedef signed short       s16;
typedef unsigned int       u32;
typedef signed int         s32;
typedef unsigned long long u64;
typedef signed long long   s64;

typedef u8  uint8_t;
typedef s8  int8_t;
typedef u16 uint16_t;
typedef s16 int16_t;
typedef u32 uint32_t;
typedef s32 int32_t;
typedef u64 uint64_t;
typedef s64 int64_t;

/* ---------------- pointer / size types (for 32-bit target) ------------- */
/* If you plan to support 64-bit later, change these typedefs accordingly */
typedef unsigned int uintptr_t;
typedef signed   int intptr_t;

typedef unsigned int size_t;
typedef int          ssize_t;

/* semantic names for addresses */
typedef uintptr_t phys_addr_t;
typedef uintptr_t virt_addr_t;

typedef void *type_t;
/* ---------------- boolean (freestanding) ---------------- */
#ifndef __cplusplus
/* prefer _Bool if available (C99), else fallback */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#include <stdbool.h>
typedef bool bool_t;
#else
typedef unsigned char bool_t;
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#endif
#else
/* in C++ bool exists */
typedef bool bool_t;
#endif

/* ---------------- NULL ---------------- */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ---------------- offsetof (safe) ---------------- */
#ifndef offsetof
#define offsetof(type, member) ((size_t)&(((type *)0)->member))
#endif

/* ---------------- container_of ---------------- */
#ifndef container_of
#define container_of(ptr, type, member) \
    ((type *)(((char *)(ptr)) - offsetof(type, member)))
#endif

/* ---------------- common utility macros ---------------- */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define BIT(n) (1U << (n))

/* Alignment helpers (align must be power of two) */
#define ALIGN_UP(x, align)   ((((uintptr_t)(x)) + ((align) - 1)) & ~((uintptr_t)((align) - 1)))
#define ALIGN_DOWN(x, align) (((uintptr_t)(x)) & ~((uintptr_t)((align) - 1)))

/* Packed / aligned attributes */
#if defined(__GNUC__) || defined(__clang__)
#define PACKED __attribute__((packed))
#define ALIGNED(n) __attribute__((aligned(n)))
#else
#define PACKED
#define ALIGNED(n)
#endif

/* portable always inline */
#ifndef __always_inline
#if defined(__GNUC__) || defined(__clang__)
#define __always_inline static inline __attribute__((always_inline))
#else
#define __always_inline static inline
#endif
#endif

/* safer MIN/MAX: avoid double evaluation when GNU typeof is available */
#if (defined(__GNUC__) || defined(__clang__))
#define MIN(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b; })
#define MAX(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })
#else
/* Warning: may evaluate arguments multiple times */
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#endif

/* ---------------- alignment-safe static assert ---------------- */
/* Use _Static_assert when available; otherwise fallback to line-based typedef */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define STATIC_ASSERT(expr, msg) _Static_assert((expr), msg)
#else
/* create unique identifier using __LINE__ */
#define STATIC_ASSERT_CONCAT(a, b) a##b
#define STATIC_ASSERT_LINE(name, line) STATIC_ASSERT_CONCAT(name, line)
#define STATIC_ASSERT(expr, msg) \
    typedef char STATIC_ASSERT_LINE(static_assertion_, __LINE__)[(expr) ? 1 : -1]
#endif

/* ---------------- basic size assertions for 32-bit target ---------------- */
STATIC_ASSERT(sizeof(u8) == 1, "u8_must_be_1_byte");
STATIC_ASSERT(sizeof(u16) == 2, "u16_must_be_2_bytes");
STATIC_ASSERT(sizeof(u32) == 4, "u32_must_be_4_bytes");
STATIC_ASSERT(sizeof(uintptr_t) == 4, "uintptr_t_must_be_4_bytes");

/* ---------------- size constants ---------------- */
#define KIB (1024U)
#define MIB (1024U * KIB)
#define GIB (1024U * MIB)

/* ---------------- small helper: UNUSED macro ---------------- */
#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_TYPES_H */
