#include "gdt.h"
#include "vga.h"
#include "string.h"

// ==================== GDT 全局变量 ====================

// 定义最多 7 个 GDT 表项：空、代码段、数据段、视频段，用户代码、用户数据、TSS
#define GDT_ENTRY_COUNT 7

static gdt_entry_t gdt[GDT_ENTRY_COUNT] __attribute__((aligned(16)));
static gdt_ptr_t gdt_ptr;
static struct tss_struct tss __attribute__((aligned(16)));

// 声明外部汇编函数
extern void gdt_flush(uint32_t gdt_ptr_addr);
extern void tss_flush();

// ==================== 工具函数 ====================

// 设置一个 GDT 表项
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_mid = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = limit & 0xFFFF;
    /* gran: high nibble (bits 7..4) hold G/D/L/AVL */
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

// ==================== TSS ====================

void tss_set_stack(uint32_t stack)
{
    tss.esp0 = stack;
}

// ==================== GDT 初始化 ====================

void gdt_init()
{
    memset(&gdt, 0, sizeof(gdt));
    memset(&tss, 0, sizeof(tss));

    gdt_ptr.limit = sizeof(gdt_entry_t) * GDT_ENTRY_COUNT - 1;
    gdt_ptr.base = (uint32_t)&gdt;

    // Null Descriptor
    gdt_set_gate(0, 0, 0, 0, 0);

    // 内核代码段
    gdt_set_gate(1, 0, 0xFFFFF, GDT_CODE_KERNEL, GDT_FLAGS_KERNEL);

    // 内核数据段
    gdt_set_gate(2, 0, 0xFFFFF, GDT_DATA_KERNEL, GDT_FLAGS_KERNEL);

    // 视频段
    gdt_set_gate(3, 0x000B8000, 0x07FFF, GDT_DATA_KERNEL, GDT_FLAGS_VEDIO);

    // 用户代码段
    gdt_set_gate(4, 0, 0xFFFFF, GDT_CODE_USER, GDT_FLAGS_USER);

    // 用户数据段
    gdt_set_gate(5, 0, 0xFFFFF, GDT_DATA_USER, GDT_FLAGS_USER);

    // 设置 TSS
    uint32_t base = (uint32_t)&tss;
    // uint32_t limit = base + sizeof(tss);
    uint32_t limit = sizeof(tss) - 1;
    gdt_set_gate(6, base, limit, GDT_TSS_AVAIL, GDT_FLAGS_TSS);

    // 初始化 TSS
    tss.cs = GDT_KERNEL_CODE_SEL;                                       // 内核代码段选择子
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = GDT_KERNEL_DATA_SEL;   // 内核数据段选择子
    tss.ss0 = GDT_KERNEL_DATA_SEL;  // 内核数据段选择子
    tss.esp0 = GDT_TSS_ESP0;        // 内核栈在稍后初始化
    tss.iomap_base = sizeof(tss);

    gdt_flush((uint32_t)&gdt_ptr);

    // 加载 TSS
    tss_flush();

    vga_printf("[GDT] Initialized.\n");
}

static const char *get_access_type_str(uint8_t access)
{
    if (!(access & 0x80))
        return "Not Present";
    if (access & 0x10)
    { // Code/Data
        if (access & 0x08)
        { // Code
            return (access & 0x02) ? "Code, Readable" : "Code, Execute-Only";
        }
        else
        { // Data
            return (access & 0x02) ? "Data, Read/Write" : "Data, Read-Only";
        }
    }
    else
    { // System
        switch (access & 0x0F)
        {
        case 0x9:
            return "TSS (Available 32-bit)";
        case 0xB:
            return "TSS (Busy 32-bit)";
        default:
            return "System (Other)";
        }
    }
}

static void print_gdt_entry(int i, gdt_entry_t *entry)
{
    uint32_t base = entry->base_low | ((uint32_t)entry->base_mid << 16) | ((uint32_t)entry->base_high << 24);
    uint32_t limit = entry->limit_low | ((uint32_t)entry->granularity & 0x0F) << 16;
    uint8_t access = entry->access;
    uint8_t gran = entry->granularity;

    vga_printf("GDT[%d]: Base=0x%x, Limit=0x%x, Access=0x%x, Gran=0x%x\n",
               i, base, limit, access, gran);
    vga_printf("        -> Type: %s, DPL=%d, Granularity=%s, Size=%s\n",
               get_access_type_str(access),
               (access >> 5) & 0x3,
               (gran & 0x80) ? "4K" : "1B",
               (gran & 0x40) ? "32-bit" : "16-bit");
}

// ==================== GDT 调试函数 ====================

void gdt_dump(void)
{
    vga_printf("\n--- GDT Dump ---\n");
    vga_printf("GDT Base: 0x%x, Limit: 0x%x\n", gdt_ptr.base, gdt_ptr.limit);

    for (int i = 0; i < GDT_ENTRY_COUNT; i++)
    {
        print_gdt_entry(i, &gdt[i]);
    }

    vga_printf("\n--- CPU Segment Registers ---\n");
    uint16_t cs, ds, es, fs, gs, ss;
    __asm__ volatile("mov %%cs, %0" : "=r"(cs));
    __asm__ volatile("mov %%ds, %0" : "=r"(ds));
    __asm__ volatile("mov %%es, %0" : "=r"(es));
    __asm__ volatile("mov %%fs, %0" : "=r"(fs));
    __asm__ volatile("mov %%gs, %0" : "=r"(gs));
    __asm__ volatile("mov %%ss, %0" : "=r"(ss));

    vga_printf("CS: 0x%x, DS: 0x%x, ES: 0x%x\n", cs, ds, es);
    vga_printf("FS: 0x%x, GS: 0x%x, SS: 0x%x\n", fs, gs, ss);
    vga_printf("--------------------\n");
}