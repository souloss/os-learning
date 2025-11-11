#ifndef _GDT_H
#define _GDT_H

#include <common.h>

// GDT 表项结构
struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

// GDT 指针结构，用于 lgdt 指令
struct gdt_ptr_struct
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// TSS (任务状态段) 结构，用于提供内核栈
struct tss_struct
{
    uint32_t prev_tss; // 上一任务的TSS选择子
    uint32_t esp0;     // 内核栈顶指针 (Ring 0)
    uint32_t ss0;      // 内核栈段选择子 (Ring 0)
    uint32_t esp1;     // Ring 1栈指针
    uint32_t ss1;      // Ring 1栈段选择子
    uint32_t esp2;     // Ring 2栈指针
    uint32_t ss2;      // Ring 2栈段选择子
    uint32_t cr3;      // 页目录基址寄存器
    uint32_t eip;      // 指令指针
    uint32_t eflags;   // 标志寄存器
    uint32_t eax;      // 通用寄存器
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp; // 栈指针
    uint32_t ebp; // 基址指针
    uint32_t esi; // 源变址寄存器
    uint32_t edi; // 目的变址寄存器
    uint32_t es;  // 段寄存器
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;        // LDT选择子
    uint16_t trap;       // 调试陷阱标志
    uint16_t iomap_base; // I/O权限位图基址
} __attribute__((packed));

// 初始化 GDT
void gdt_init();

// 设置 GDT 表项
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

// 设置 TSS
void tss_set_stack(uint32_t stack);

#endif