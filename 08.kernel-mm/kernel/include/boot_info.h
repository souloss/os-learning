#ifndef BOOT_INFO_H
#define BOOT_INFO_H

#include <stdint.h>

#define KERNEL_SPACE_START 0xC0000000
#define E820_MAX_ENTRIES 128
#define E820_BUFFER_PHYS 0x9000
#define E820_SIGNATURE 0x534D4150  /* 'SMAP' */
#define BOOT_INFO_MAGIC 0x1BADB002 /* Magic number for sanity check */

typedef struct e820_entry
{
    uint64_t addr; /* 内存区域的起始地址 */
    uint64_t size; /* 内存区域的大小 */
    uint32_t type; /* 内存区域的类型 */
    uint32_t acpi; /* 内存区域的类型 */
} __attribute__((packed)) e820_entry_t;

/**
 * 链接器段信息
 */
typedef struct
{
    uint32_t text_start;
    uint32_t text_end;
    uint32_t rodata_start;
    uint32_t rodata_end;
    uint32_t data_start;
    uint32_t data_end;
    uint32_t bss_start;
    uint32_t bss_end;
    uint32_t kernel_end;
    uint32_t kernel_size;
    uint32_t kernel_phys_base;
    uint32_t kernel_virt_base;
} kernel_section_info_t;

/**
 * Boot 信息结构体（MBR/Loader 填充，内核读取）
 */
typedef struct
{
    uint32_t magic;      /* 校验用 */
    uint32_t e820_count; /* 有效 E820 条目数 */
    e820_entry_t e820_map[E820_MAX_ENTRIES];
    kernel_section_info_t kernel_sections; /* 链接器段信息 */
} boot_info_t;

extern boot_info_t boot_info;

/* 初始化函数：读取 E820 + 链接符号信息 */
void boot_info_init(void);

/* 调试打印 */
void boot_info_dump(void);

#endif
