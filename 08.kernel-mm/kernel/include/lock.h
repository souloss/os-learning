#ifndef LOCK_H
#define LOCK_H

#include "types.h"

/* 原子原语 */
uint32_t atomic_exchange(volatile uint32_t *ptr, uint32_t val);
uint32_t compare_and_exchange(volatile uint32_t *ptr,
                              uint32_t expected,
                              uint32_t newval);
uint32_t atomic_fetch_add(volatile uint32_t *ptr, int32_t addend);
uint32_t atomic_inc(volatile uint32_t *ptr);
uint32_t atomic_dec(volatile uint32_t *ptr);

/* 屏障 */
void barrier(void);
void mb(void);
void rmb(void);
void wmb(void);

/* 中断控制 */
uint32_t get_eflags(void);
void set_eflags(uint32_t eflags);
void cpu_cli(void);
void cpu_sti(void);
uint32_t cpu_save_flags_and_cli(void);

#endif