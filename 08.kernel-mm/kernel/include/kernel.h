#ifndef _KERNEL_H
#define _KERNEL_H

#include "types.h"
#include "interrupt.h"
#include "vga.h"

// 内核 Panic 函数 
__attribute__((noreturn)) static inline void kernel_panic(const char* file, const char* func, int line)
{
    disable_interrupts();
    vga_printf("KERNEL PANIC !!!\n ");
    vga_printf(" %s, %s(), line %d\n", file, func, line);
    vga_printf("System halted.\n");
    for (;;)
    {
        __asm__ volatile("hlt");
    }
}

#define PANIC() kernel_panic(__FILE__, __FUNCTION__, __LINE__)

#ifdef NDEBUG
#define ASSERT(CONDITION) ((void)0)
#else
#define ASSERT(CONDITION) \
    if (CONDITION)        \
    {                     \
    }                     \
    else                  \
    {                     \
        PANIC();          \
    }
#endif // NDEBUG

#endif // _KERNEL_H