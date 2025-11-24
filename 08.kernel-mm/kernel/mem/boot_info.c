#include "boot_info.h"
#include "vga.h"

/* 来自汇编/链接脚本的符号（由 ld 自动导出） */
extern char kernel_text_start[], kernel_text_end[];
extern char kernel_rodata_start[], kernel_rodata_end[];
extern char kernel_data_start[], kernel_data_end[];
extern char kernel_bss_start[], kernel_bss_end[];
extern char kernel_end[];
extern char kernel_size[];
extern char kernel_phys_offset[];
extern char kernel_virt_base[];

boot_info_t boot_info = {0};
#define E820_BUFFER_VIRT (KERNEL_SPACE_START + E820_BUFFER_PHYS)

void boot_info_init(void)
{
    boot_info.magic = BOOT_INFO_MAGIC;

    e820_entry_t *map = (e820_entry_t *)(uintptr_t)E820_BUFFER_VIRT;

    uint32_t count = 0;
    for (; count < E820_MAX_ENTRIES; count++)
    {
        e820_entry_t *e = &map[count];

        /* 当 addr、size、type 都为 0 时，视为结束 */
        if (e->addr == 0 && e->size == 0 && e->type == 0)
            break;

        boot_info.e820_map[count] = *e;
    }
    boot_info.e820_count = count;
    /* 3. 填充链接器信息 */
    boot_info.kernel_sections.text_start = (uint32_t)kernel_text_start;
    boot_info.kernel_sections.text_end = (uint32_t)kernel_text_end;
    boot_info.kernel_sections.rodata_start = (uint32_t)kernel_rodata_start;
    boot_info.kernel_sections.rodata_end = (uint32_t)kernel_rodata_end;
    boot_info.kernel_sections.data_start = (uint32_t)kernel_data_start;
    boot_info.kernel_sections.data_end = (uint32_t)kernel_data_end;
    boot_info.kernel_sections.bss_start = (uint32_t)kernel_bss_start;
    boot_info.kernel_sections.bss_end = (uint32_t)kernel_bss_end;
    boot_info.kernel_sections.kernel_end = (uint32_t)kernel_end;
    boot_info.kernel_sections.kernel_size = (uint32_t)kernel_size;
    boot_info.kernel_sections.kernel_phys_base = (uint32_t)kernel_phys_offset;
    boot_info.kernel_sections.kernel_virt_base = (uint32_t)kernel_virt_base;
}

void boot_info_dump(void)
{
    vga_printf("BootInfo (magic=0x%x)\n", boot_info.magic);
    vga_printf("E820 entries: %d\n", boot_info.e820_count);
    for (uint32_t i = 0; i < boot_info.e820_count; ++i)
    {
        e820_entry_t *e = &boot_info.e820_map[i];
        vga_printf(" [%d] base=0x%llx len=0x%llx type=%d\n",
                   i, e->addr, e->size, e->type);
    }

    kernel_section_info_t *k = &boot_info.kernel_sections;
    vga_printf("Kernel Sections:\n");
    vga_printf(" text:   [0x%x - 0x%x)\n", k->text_start, k->text_end);
    vga_printf(" rodata: [0x%x - 0x%x)\n", k->rodata_start, k->rodata_end);
    vga_printf(" data:   [0x%x - 0x%x)\n", k->data_start, k->data_end);
    vga_printf(" bss:    [0x%x - 0x%x)\n", k->bss_start, k->bss_end);
    vga_printf(" kernel_end=%x size=%h\n", k->kernel_end, k->kernel_size);
}
