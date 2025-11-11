#ifndef _GDT_H
#define _GDT_H

#include <types.h>

// ---------------------- Access Byte (bits 8..15) ----------------------
// bit 7: P 段存在位
#define GDT_ACCESS_P 0x80 // Present = 1

// bits 6-5: DPL 特权级
#define GDT_ACCESS_DPL0 0x00 // 内核级 0
#define GDT_ACCESS_DPL1 0x20
#define GDT_ACCESS_DPL2 0x40
#define GDT_ACCESS_DPL3 0x60 // 用户级 3

// bit 4: S 描述符类型
#define GDT_ACCESS_S_SYSTEM 0x00   // 系统段 (TSS, LDT)
#define GDT_ACCESS_S_CODEDATA 0x10 // 代码/数据段

// bits 3-0: TYPE 字段
// --- 数据段类型（S=1 且 TYPE[3]=0）---
#define GDT_DATA_RD 0x00   // 只读
#define GDT_DATA_RDA 0x01  // 只读 + 已访问
#define GDT_DATA_RW 0x02   // 可读写
#define GDT_DATA_RWA 0x03  // 可读写 + 已访问
#define GDT_DATA_EXP 0x04  // 向下扩展，只读
#define GDT_DATA_EXPW 0x06 // 向下扩展，可写

// --- 代码段类型（S=1 且 TYPE[3]=1）---
#define GDT_CODE_X 0x08             // 只执行
#define GDT_CODE_XR 0x0A            // 可执行 + 可读（最常用）
#define GDT_CODE_XC 0x0C            // 一致代码段
#define GDT_CODE_XRC 0x0E           // 可执行 + 可读 + 一致
#define GDT_TYPE_TSS_AVAIL    0x09  // 32位可用TSS
// ---------------------- Flags (bits 20..23) ----------------------
// bit 7 (in granularity byte) = G
#define GDT_GRAN_4K 0x80 // 粒度 = 4KB
#define GDT_GRAN_1B 0x00 // 粒度 = 1B
// bit 6 = D/B (default operand size)
#define GDT_OP_SIZE_32 0x40 // 32 位段
#define GDT_OP_SIZE_16 0x00 // 16 位段
// bit 5 = L (64-bit code segment)
#define GDT_LONG_MODE 0x20 // 仅用于 x86_64 模式
// bits 4-0 = 高 4 位 limit
#define GDT_LIMIT_HIGH(x) (((x) >> 16) & 0x0F)

// ---------------------- 常用组合 ----------------------
#define GDT_CODE_KERNEL (GDT_ACCESS_P | GDT_ACCESS_DPL0 | GDT_ACCESS_S_CODEDATA | GDT_CODE_XR)
#define GDT_DATA_KERNEL (GDT_ACCESS_P | GDT_ACCESS_DPL0 | GDT_ACCESS_S_CODEDATA | GDT_DATA_RW)
#define GDT_CODE_USER   (GDT_ACCESS_P | GDT_ACCESS_DPL3 | GDT_ACCESS_S_CODEDATA | GDT_CODE_XR)
#define GDT_DATA_USER   (GDT_ACCESS_P | GDT_ACCESS_DPL3 | GDT_ACCESS_S_CODEDATA | GDT_DATA_RW)
#define GDT_TSS_AVAIL   (GDT_ACCESS_P | GDT_ACCESS_DPL0 | GDT_ACCESS_S_SYSTEM | GDT_TYPE_TSS_AVAIL)

// 粒度组合
#define GDT_FLAGS_KERNEL (GDT_GRAN_4K | GDT_OP_SIZE_32)
#define GDT_FLAGS_USER   (GDT_GRAN_4K | GDT_OP_SIZE_32)
#define GDT_FLAGS_VEDIO  (GDT_GRAN_1B | GDT_OP_SIZE_16)
#define GDT_FLAGS_TSS    (GDT_GRAN_1B | GDT_OP_SIZE_32)

// 选择子定义
#define GDT_SELECTOR(index, rpl) ((index << 3) | (rpl & 0x3))

// 常用段选择子
#define GDT_KERNEL_CODE_SEL GDT_SELECTOR(1, 0)
#define GDT_KERNEL_DATA_SEL GDT_SELECTOR(2, 0)
#define GDT_VIDEO_SEL       GDT_SELECTOR(3, 0)
#define GDT_USER_CODE_SEL   GDT_SELECTOR(4, 3)
#define GDT_USER_DATA_SEL   GDT_SELECTOR(5, 3)
#define GDT_TSS_SEL         GDT_SELECTOR(6, 0)
#define GDT_TSS_ESP0        0xF0000000-16

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

typedef struct gdt_entry gdt_entry_t;

// GDT 指针结构，用于 lgdt 指令
struct gdt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

typedef struct gdt_ptr gdt_ptr_t;

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

typedef struct tss_struct tss_t;

// 初始化 GDT
void gdt_init();

// 设置 GDT 表项
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

// 设置 TSS
void tss_set_stack(uint32_t stack);

void gdt_dump(void);

#endif