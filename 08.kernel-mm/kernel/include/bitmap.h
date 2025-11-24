#ifndef _BITMAP_H
#define _BITMAP_H

#include "types.h"

// 位图结构体
typedef struct
{
    uint32_t *bits; // 指向位图数据区
    uint32_t size;  // 位图的总位数
} bitmap_t;

// 初始化一个位图
void bitmap_init(bitmap_t *map, uint32_t *bits_buffer, uint32_t num_bits);

// 设置位
static inline void bitmap_set_bit(bitmap_t *map, uint32_t bit)
{
    map->bits[bit / 32] |= (1 << (bit % 32));
}

// 清除位
static inline void bitmap_clear_bit(bitmap_t *map, uint32_t bit)
{
    map->bits[bit / 32] &= ~(1 << (bit % 32));
}

// 测试位是否被设置
static inline bool bitmap_test_bit(bitmap_t *map, uint32_t bit)
{
    return map->bits[bit / 32] & (1 << (bit % 32));
}

// 查找第一个空闲的位并设置它，返回位的索引；如果没有空闲位，返回 (uint32_t)-1
uint32_t bitmap_find_and_set_first_free(bitmap_t *map);

#endif // _BITMAP_H