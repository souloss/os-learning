#include "timer.h"
#include "interrupt.h"
#include "ports.h"
#include "io.h"
#include "vga.h"

uint32_t tick = 0;

uint32_t getTick()
{
    return tick;
}

static void timer_callback(interrupt_frame_t *regs)
{
    tick++;
    vga_printf("tick=%d\n", tick);
}

void init_timer(uint32_t frequency)
{
    // register our timer callback.
    register_interrupt_handler(IRQ0_INT_NUM, &timer_callback);

    if (frequency == 0)
        frequency = 1;
    // the divisor must be small enough to fit into 16-bits.
    uint32_t divisor = (CLOCK_TICK_RATE + frequency / 2) / frequency;

    // send the command byte.
    outb(PIT_CMD, PIT_CHANNEL0 | PIT_MODE3 | PIT_ACCESS_BOTH | PIT_BCD);

    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);

    // Send the frequency divisor.
    outb(PIT_CH0, l);
    outb(PIT_CH0, h);
}
