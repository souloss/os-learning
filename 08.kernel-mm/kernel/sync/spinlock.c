#include "spinlock.h"
#include "lock.h"
#include "interrupt.h"

extern uint32_t atomic_exchange(volatile uint32_t* dst, uint32_t src);
extern uint32_t get_eflags();

/**
 * spinlock_init - 初始化自旋锁
 * @splock: 要初始化的锁对象
 *
 * 单核场景下：
 * 1. lock 字段：0 表示没人持有
 * 2. interrupt_mask：记录“本锁”在加锁时中断是否开启，用于解锁时精确恢复
 *    （只给 spin_lock_irqsave 系列使用）
 */
void spinlock_init(spinlock_t *splock)
{
  splock->lock = LOCKED_NO;
  splock->interrupt_mask = 0;
}

/**
 * spinlock_lock - 纯线程间抢锁（**不能**在 ISR 里使用）
 * @splock: 锁对象
 *
 * 单核关键点：
 * 1. 只有一个 CPU，不存在“另一个核同时写 lock”的情况。
 * 2. 线程上下文不会被并行执行，只要关抢占或关中断即可保证互斥。
 * 3. 因此解锁时直接用普通赋值“splock->lock = LOCKED_NO”是安全的。
 *
 * 这里用 atomic_exchange 只是为了“忙等 + 原子读-改-写”语义，
 * 在单核下它其实退化成了“带等待的原子赋值”，不会真正竞争。
 */
void spinlock_lock(spinlock_t *splock)
{
  /* 如果锁已被占用，会一直空转直到持有者释放 */
  while (atomic_exchange(&splock->lock, LOCKED_YES) != LOCKED_NO)
  {
    /* 单核空转时不需要 pause，因为不会有第二个核释放锁 */
  }
}

/**
 * spinlock_lock_irqsave - 带“关中断”功能的抢锁
 * @splock: 锁对象
 *
 * 使用场景：
 * 1. 临界区可能被**中断处理程序**访问，因此必须先关中断再抢锁。
 * 2. 单核下“关中断”本身就保证了原子性，但我们仍然用 atomic_exchange
 *    完成“忙等”语义。
 *
 * 执行步骤：
 * 1. 保存当前 EFLAGS 的 IF 位（第 9 位），记录到 interrupt_mask。
 * 2. 执行 cli 关闭本地中断。
 * 3. 用原子交换抢锁；若失败继续空转（此时中断已关，不会死锁）。
 * 4. 解锁时根据 interrupt_mask 决定是否重新 sti。
 */
void spinlock_lock_irqsave(spinlock_t *splock)
{
  uint32_t eflags = get_eflags();               /* 读取进入前的完整标志寄存器 */
  disable_interrupts();                         /* cli：本地 CPU 不再响应 IRQ */
  splock->interrupt_mask = (eflags & (1 << 9)); /* 只保留 IF 位 */

  /* 下面与 spinlock_lock 完全相同 */
  while (atomic_exchange(&splock->lock, LOCKED_YES) != LOCKED_NO)
  {
  }
}

/**
 * spinlock_unlock - 释放锁（**纯线程**版本）
 * @splock: 锁对象
 *
 * 单核下为什么可以普通赋值？
 * 1. 关中断后唯一执行流就是当前代码，不会有并发写。
 * 2. 即使线程被抢占，只要调度器保证不会把锁让给另一个线程同时解锁，
 *    这里就仍然是串行的。
 *
 * 注意：如果将来要支持多核，必须把这句换成原子写（例如 atomic_exchange）。
 */
void spinlock_unlock(spinlock_t *splock)
{
  splock->lock = LOCKED_NO;
}

/**
 * spinlock_unlock_irqrestore - 释放锁并恢复中断状态
 * @splock: 锁对象
 *
 * 单核安全点：
 * 1. 同样因为“关中断后只有我在跑”，普通赋值足够。
 * 2. 只用“splock->interrupt_mask”决定是否 sti，保证“谁关谁开”，
 *    不会误把外层已关闭的中断重新打开。
 *
 * 恢复顺序：
 * 1. 先放锁 → 2. 再恢复中断，防止在临界区里被中断插进来。
 */
void spinlock_unlock_irqrestore(spinlock_t *splock)
{
  splock->lock = LOCKED_NO; /* 放锁 */
  if (splock->interrupt_mask)
  {                      /* 如果进锁前 IF==1，才重新开中断 */
    enable_interrupts(); /* sti */
  }
}