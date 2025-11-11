#ifndef TIMER_H
#define TIMER_H

#include "types.h"

/**
 * 软件期望的定时中断频率（单位：Hz）
 *
 * 表示“每秒中断多少次”，是你希望 PIT 产生 IRQ0 中断的目标频率。
 * 例如：
 *   TIMER_FREQUENCY = 50   → 每秒 50 次中断（每 20ms 一次）
 *   TIMER_FREQUENCY = 100  → 每秒 100 次中断（每 10ms 一次）
 *   TIMER_FREQUENCY = 1000 → 每秒 1000 次中断（每 1ms 一次）
 *
 * 注意事项：
 * 1. PIT 只支持 16 位分频值（1~65535），
 *    所以可产生的最低频率约为：
 *        CLOCK_TICK_RATE / 65535 ≈ 18.2 Hz
 *    如果你设置的 TIMER_FREQUENCY < 18Hz，
 *    实际上仍会以 18Hz 左右的速率运行。
 *
 * 2. 若你希望“一秒钟触发一次”，
 *    请不要直接设置为 1 Hz（因为超出PIT范围），
 *    而应设置为 18 或更高频率，并在软件层累加 tick 来实现 1秒节拍。
 *
 * 3. 若想获得更低或更高精度的定时（<18Hz 或 >1kHz），
 *    请考虑使用 APIC Timer 或 HPET 等高精度定时器替代 PIT。
 */
#define TIMER_FREQUENCY 50

/**
 * PIT（Intel 8253/8254）定时器的输入基准频率（单位：Hz）
 *
 * 一般PC主板的PIT使用1.193182 MHz的晶振作为输入时钟源，
 * 即每秒输入约 1193181~1193182 次脉冲。
 *
 * 若移植到不同硬件平台（例如QEMU仿真环境、嵌入式SoC）
 * 或者使用非标准晶振，请根据具体平台修改此常量为正确的频率。
 *
 * 典型值：
 *   - 标准PC/兼容机：1193182 Hz
 *   - 有些资料或平台取整为：1193180 Hz
 *
 * 若此值不准确，定时器的中断频率会有微小误差，
 * 比如预期 50 Hz 实际可能是 49.998 Hz 或 50.002 Hz。
 */
#define CLOCK_TICK_RATE 1193180

void init_timer(uint32_t frequency);

uint32_t getTick();

#endif
