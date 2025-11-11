#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

/*
 * types.h
 * 基础类型定义 —— 适用于 32-bit x86 裸机/内核环境
 *
 * 说明：
 *  - 假定目标为 32-bit (i386)。若移植到 64-bit，请调整 uintptr_t / phys_addr_t / size_t 的 typedef。
 *  - 这是一个极简、独立的头文件，不依赖 C 标准库（适合 freestanding 环境）。
 */

/* ---------- 固定宽度整数类型（便于可读性） ---------- */
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long long u64;
typedef signed long long s64;
/* ---------- 兼容 stdint 风格的别名 ---------- */
typedef u8 uint8_t;
typedef s8 int8_t;
typedef u16 uint16_t;
typedef s16 int16_t;
typedef u32 uint32_t;
typedef s32 int32_t;
typedef u64 uint64_t;
typedef s64 int64_t;

/* ---------- 指针相关与大小类型（针对 32-bit） ---------- */
/* 在 32-bit 环境下，uintptr_t 与 size_t 使用 32-bit 宽度 */
typedef unsigned int uintptr_t;
typedef signed int intptr_t;

typedef unsigned int size_t;
typedef int ssize_t;

/* 物理/虚拟地址类型（便于代码可读性） */
typedef uintptr_t phys_addr_t; /* 物理地址表示 */
typedef uintptr_t virt_addr_t; /* 虚拟地址表示 */

/* 布尔类型（内核中通常使用 int/char，但提供别名便于阅读） */
typedef unsigned char bool_t;
#define TRUE 1
#define FALSE 0
#define NULL 0

/* NULL */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ---------- 常用宏 ---------- */

/* 计算数组元素数量 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* 位操作 */
#define BIT(n) (1U << (n))

/* 最小/最大 */
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

/* 对齐：向上对齐到 align（align 必须是 2 的幂） */
#define ALIGN_UP(x, align) ((((uintptr_t)(x)) + ((align) - 1)) & ~((uintptr_t)((align) - 1)))
#define ALIGN_DOWN(x, align) (((uintptr_t)(x)) & ~((uintptr_t)((align) - 1)))

/* 强制内联 / 不被内联（根据编译器） */
#ifndef __always_inline
#if defined(__GNUC__) || defined(__clang__)
#define __always_inline static inline __attribute__((always_inline))
#else
#define __always_inline static inline
#endif
#endif

/* 打包（packed）与对齐属性宏 */
#if defined(__GNUC__) || defined(__clang__)
#define PACKED __attribute__((packed))
#define ALIGNED(n) __attribute__((aligned(n)))
#else
#define PACKED
#define ALIGNED(n)
#endif

/* container_of 宏（常用于内核数据结构） */
#ifndef container_of
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

/* offsetof（如果没有 stddef.h 时提供） */
#ifndef offsetof
#define offsetof(type, member) ((size_t)&(((type *)0)->member))
#endif

/* ---------- 简单静态断言（仅当支持 C11 _Static_assert 时生效） ---------- */
#ifdef __STDC_VERSION__
#if __STDC_VERSION__ >= 201112L
#define STATIC_ASSERT(expr, msg) _Static_assert((expr), msg)
#else
#define STATIC_ASSERT(expr, msg) typedef char static_assertion_##msg[(expr) ? 1 : -1]
#endif
#else
#define STATIC_ASSERT(expr, msg) typedef char static_assertion_##msg[(expr) ? 1 : -1]
#endif

/* ---------- 常见静态断言示例（可按需启用/修改） ---------- */
/* 保证基本类型大小符合预期（32-bit 平台） */
STATIC_ASSERT(sizeof(u8) == 1, "u8_must_be_1_byte");
STATIC_ASSERT(sizeof(u16) == 2, "u16_must_be_2_bytes");
STATIC_ASSERT(sizeof(u32) == 4, "u32_must_be_4_bytes");
STATIC_ASSERT(sizeof(uintptr_t) == 4, "uintptr_t_must_be_4_bytes");

#endif /* KERNEL_TYPES_H */
