#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <types.h>

// IDT 表项结构
struct idt_entry
{
    uint16_t base_low;
    uint16_t selector;
    uint8_t always_zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

// IDT 指针结构
struct idt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// 中断处理函数类型
typedef void (*interrupt_handler_t)(struct regs *);

// 设置 IDT
void idt_init();

// 注册中断处理函数
void register_interrupt_handler(int n, interrupt_handler_t handler);

// PIC 操作
void pic_init();
void pic_send_eoi(uint8_t irq);

#endif