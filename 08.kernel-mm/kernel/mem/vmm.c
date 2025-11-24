/**
 * @file vmm.c
 * @brief 虚拟内存管理器实现
 */

#include "vmm.h"
#include "pmm.h"
#include "vga.h"
#include "string.h"
#include "ports.h"

// ====================================================================
// 内部状态与辅助函数
// ====================================================================

/**
 * @brief 内核页目录的物理地址
 */
static uint32_t kernel_directory_phys_addr;

/**
 * @brief 获取一个虚拟地址对应的页表项
 *
 * @param virt_addr 虚拟地址
 * @return page_table_entry_t* 指向页表项的指针。
 *         如果对应的页目录项不存在，则返回 NULL。
 * @note 此函数利用自映射机制，可以访问任何当前活动页目录的页表。
 */
static page_table_entry_t *get_page(uint32_t virt_addr)
{
    // 从虚拟地址中提取页目录索引和页表索引
    uint32_t page_dir_idx = virt_addr >> 22;
    uint32_t page_tbl_idx = (virt_addr >> 12) & 0x3FF;

    // 获取页目录项
    page_directory_entry_t *pde = &((page_directory_t *)PAGE_DIR_VIRTUAL)->entries[page_dir_idx];

    // 检查页表是否存在
    if (!pde->present)
    {
        return NULL; // 页表不存在
    }

    // 【自映射核心】计算页表的虚拟地址
    // 无论 current_directory 指向哪个页目录，页表区域总是被映射到 PAGE_TABLES_VIRTUAL_ADDR
    page_table_t *page_table_virt = (page_table_t *)(PAGE_TABLES_VIRTUAL_ADDR + page_dir_idx * PAGE_SIZE);

    // 返回页表项
    return &page_table_virt->entries[page_tbl_idx];
}

/**
 * @brief 使单个页的 TLB 条目失效
 */
static inline void invalidate_page(uint32_t addr)
{
    asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

// ====================================================================
// VMM 公共接口实现
// ====================================================================

void vmm_init(void)
{
    // 1. 设置 VMM 的内部状态
    kernel_directory_phys_addr = KERNEL_PAGE_DIR_PHY;

    // 【新增】注册 Page Fault (中断 14) 处理程序
    register_interrupt_handler(INT_PAGE_FAULT, vmm_page_fault_handler);

    vga_printf("[VMM] Initialized with fixed page tables.\n");
    vga_printf("    Page Dir @ 0x%x (Virt: 0x%x)\n", kernel_directory_phys_addr, PAGE_DIR_VIRTUAL);
    vga_printf("    Page Tables @ 0x%x\n", PAGE_TABLES_VIRTUAL_ADDR);
    vga_printf("    Page Fault handler registered for on-demand paging.\n");
}

bool_t vmm_map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags)
{
    page_table_entry_t *page = get_page(virt_addr);
    if (page == NULL)
    {
        vga_printf("VMM: Failed to map page 0x%x. Page table not present.\n", virt_addr);
        return false; // 页表不存在，映射失败
    }

    // 设置页表项
    page->frame_addr = phys_addr >> 12;
    page->present = (flags & PAGE_PRESENT) ? 1 : 0;
    page->rw = (flags & PAGE_RW) ? 1 : 0;
    page->user = (flags & PAGE_USER) ? 1 : 0;
    page->writethrough = (flags & PAGE_WRITETHROUGH) ? 1 : 0;
    page->cache_disable = (flags & PAGE_CACHE_DISABLE) ? 1 : 0;

    // 刷新 TLB
    invalidate_page(virt_addr);

    return true;
}

bool_t vmm_alloc_and_map_page(uint32_t virt_addr, uint32_t flags)
{
    uint32_t new_phys_page = pmm_alloc_page();
    if (new_phys_page == 0)
        return false; // 内存耗尽

    return vmm_map_page(virt_addr, new_phys_page, flags);
}

void vmm_unmap_page(uint32_t virt_addr)
{
    page_table_entry_t *page = get_page(virt_addr);
    if (page == NULL || !page->present)
    {
        return; // 页未映射，无需操作
    }

    // 【修复】使用复合字面量将整个结构体清零
    *page = (page_table_entry_t){0};

    // 刷新 TLB
    invalidate_page(virt_addr);
}

uint32_t vmm_get_phys_addr(uint32_t virt_addr)
{
    page_table_entry_t *page = get_page(virt_addr);
    if (page == NULL || !page->present)
    {
        return 0; // 未映射
    }

    return (page->frame_addr << 12) + (virt_addr & 0xFFF);
}

void vmm_switch_page_directory(uint32_t new_directory_phys_addr)
{
    asm volatile("mov %0, %%cr3" : : "r"(new_directory_phys_addr));
}

uint32_t vmm_get_current_directory_phys_addr(void)
{
    // 【最准确的方式】直接从 CR3 寄存器读取当前页目录的物理地址
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void vmm_page_fault_handler(interrupt_frame_t *frame)
{
    // 1. 从 CR2 寄存器读取导致故障的线性地址
    uint32_t faulting_addr;
    asm volatile("mov %%cr2, %0" : "=r"(faulting_addr));

    if (faulting_addr < KERNEL_LOAD_VIRTUAL_ADDR)
    {
        vga_printf("\n!!!!! KERNEL PAGE FAULT (Null Pointer?) !!!!!\n");
        vga_printf("Faulting address: 0x%x\n", faulting_addr);
        vga_printf("Kernel accessed invalid low memory.");
        PANIC();
    }

    // 2. 对齐地址到页边界
    uint32_t aligned_addr = PAGE_ALIGN_DOWN(faulting_addr);

    if (!vmm_alloc_and_map_page(aligned_addr, PAGE_KERNEL_FLAGS)) {
        vga_printf("Page fault: Out of memory.");
        PANIC();
    }
}