#ifndef _SPINLOCK_H
#define _SPINLOCK_H

#include "types.h"

#define LOCKED_YES 1
#define LOCKED_NO 0

typedef struct
{
    volatile uint32_t lock;
    volatile uint32_t interrupt_mask;
} spinlock_t;

// 初始化锁
void spinlock_init(spinlock_t *lock);

// 获取锁
void spinlock_acquire(spinlock_t *lock);

// 释放锁
void spinlock_release(spinlock_t *lock);

// 获取锁并禁用中断
void spinlock_acquire_irq(spinlock_t *lock);

// 释放锁并恢复中断
void spinlock_release_irq(spinlock_t *lock);

#endif // _SPINLOCK_H