#include "pmm.h"
#include "boot_info.h"
#include "vga.h"
#include "string.h"
#include "kernel.h"

// ====================================================================
// 位图实现的内部状态和辅助函数
// ====================================================================
/**
 * @brief 定义位图的最大大小。
 * @details 128KB 的位图可以管理 128KB * 8 * 4KB = 4GB 的物理内存。
 *          对于学习和早期开发，这个值足够了。
 */
#define MAX_BITMAP_SIZE_BYTES (128 * 1024)

/**
 * @brief 低端保留内存的大小。
 * @note 传统PC架构中，低1MB内存包含BIOS数据区和硬件映射I/O，
 *       必须被无条件保留，无论E820如何报告。1MB - 2MB 空间用于存放页表
 */
#define LOW_MEMORY_SIZE (2 * MIB)

/**
 * @brief 静态数组作为位图的存储空间
 * @note 这个数组位于 .bss 段，其虚拟地址在编译时就已确定，且保证有效。
 */
static uint8_t pmm_bitmap[MAX_BITMAP_SIZE_BYTES];

/**
 * @brief 位图实际使用的大小（字节）
 */
static uint32_t pmm_bitmap_size_bytes = 0;

/**
 * @brief 系统管理的总物理页数（由最高物理地址决定）
 * @note 此值决定了位图的大小，覆盖整个物理地址空间。
 */
static uint32_t pmm_total_pages = 0;

/**
 * @brief 可分配的物理内存（RAM）的最大页号
 * @note pmm_alloc_page 的搜索范围是 [0, pmm_max_ram_page)。
 */
static uint32_t pmm_max_ram_page = 0;

/**
 * @brief 当前空闲的物理页数量
 */
static uint32_t pmm_free_pages = 0;

/**
 * @brief 上一次分配页的索引，用于 Next Fit 策略
 */
static uint32_t pmm_last_alloc_index = 0;

/**
 * @brief 在位图中设置指定位（标记为已使用）
 * @param bit 页的索引（从0开始）
 */
static inline void pmm_set_bit(uint32_t bit)
{
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

/**
 * @brief 在位图中清除指定位（标记为空闲）
 * @param bit 页的索引（从0开始）
 */
static inline void pmm_clear_bit(uint32_t bit)
{
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

/**
 * @brief 测试指定位的状态
 * @param bit 页的索引（从0开始）
 * @return true 如果该页已被使用
 * @return false 如果该页是空闲的
 */
static inline bool_t pmm_test_bit(uint32_t bit)
{
    return pmm_bitmap[bit / 8] & (1 << (bit % 8));
}

// ====================================================================
// PMM 公共接口实现
// ====================================================================

/**
 * @brief 强制将一个物理地址范围标记为已使用
 * @param start_paddr 起始物理地址
 * @param size 区域大小（字节）
 * @note 此函数会正确地更新 pmm_free_pages 计数器，避免重复标记。
 *       如果区域超出 PMM 管理范围，则安全地忽略。
 */
static void pmm_mark_region_used(uint64_t start_paddr, uint64_t size)
{
    if (start_paddr / PAGE_SIZE >= pmm_total_pages)
        return;

    uint32_t start_page = (uint32_t)(start_paddr / PAGE_SIZE);
    uint64_t end_addr = start_paddr + size;
    uint32_t end_page = (uint32_t)MIN((end_addr + PAGE_SIZE - 1) / PAGE_SIZE, 
                                       (uint64_t)pmm_total_pages);
    for (uint32_t p = start_page; p < end_page; ++p)
    {
        if (!pmm_test_bit(p))
        {
            pmm_set_bit(p);
            pmm_free_pages--;
        }
    }
}

void pmm_init(boot_info_t *boot_info)
{
    uint64_t max_phys_addr = 0; // 用于计算位图大小 (地址空间上限)
    uint64_t max_ram_addr = 0;  // 用于计算分配上限 (RAM上限)

    if (boot_info->magic != BOOT_INFO_MAGIC)
    {
        vga_printf("PMM: Invalid boot_info magic!\n");
        PANIC();
    }

    // 1. 遍历所有 E820 条目，计算物理地址空间的上限
    for (uint32_t i = 0; i < boot_info->e820_count; ++i)
    {
        e820_entry_t *entry = &boot_info->e820_map[i];
        uint64_t entry_end = entry->addr + entry->size;
        if (entry_end > max_phys_addr)
        {
            max_phys_addr = entry_end;
        }
    }

    // 2. 遍历 E820 中的 RAM 条目，计算可分配内存的上限
    for (uint32_t i = 0; i < boot_info->e820_count; ++i)
    {
        e820_entry_t *entry = &boot_info->e820_map[i];
        if (entry->type == 1) // 只关心 RAM
        {
            uint64_t entry_end = entry->addr + entry->size;
            if (entry_end > max_ram_addr)
            {
                max_ram_addr = entry_end;
            }
        }
    }

    // 3. 设置关键变量
    pmm_total_pages = (uint32_t)(max_phys_addr / PAGE_SIZE);
    pmm_max_ram_page = (uint32_t)(max_ram_addr / PAGE_SIZE);
    pmm_bitmap_size_bytes = (pmm_total_pages + 7) / 8;

    // 4. 检查位图空间
    if (pmm_bitmap_size_bytes > MAX_BITMAP_SIZE_BYTES)
    {
        vga_printf("PMM: Bitmap space insufficient!");
        PANIC();
    }

    // 5. 初始化位图为“所有页空闲”
    memset(pmm_bitmap, 0x00, pmm_bitmap_size_bytes);
    pmm_free_pages = pmm_total_pages;

    // 6. 锁定：将所有非 RAM (type != 1) 区域标记为已使用
    for (uint32_t i = 0; i < boot_info->e820_count; ++i)
    {
        e820_entry_t *entry = &boot_info->e820_map[i];
        if (entry->type != 1)
        {
            pmm_mark_region_used(entry->addr, entry->size);
        }
    }

    // 7. 锁定：标记内核自身占用的区域
    pmm_mark_region_used(boot_info->kernel_sections.kernel_phys_base,
                         boot_info->kernel_sections.kernel_size);
    // 8. 锁定：强制保留低1MB内存
    pmm_mark_region_used(0, LOW_MEMORY_SIZE);
}

uint32_t pmm_alloc_page(void)
{
    if (pmm_free_pages == 0 || pmm_max_ram_page == 0)
        return 0;

    // 确保起始点有效
    uint32_t start = pmm_last_alloc_index % pmm_max_ram_page;
    uint32_t i = start;

    do
    {
        // 跳过低1MB区域
        if (i >= (LOW_MEMORY_SIZE / PAGE_SIZE) && !pmm_test_bit(i))
        {
            pmm_set_bit(i);
            pmm_free_pages--;
            pmm_last_alloc_index = i;
            return i * PAGE_SIZE;
        }
        i = (i + 1) % pmm_max_ram_page;
    } while (i != start);

    return 0;
}

void pmm_free_page(uint32_t paddr)
{
    if (paddr % PAGE_SIZE != 0)
    {
        return; // 地址未对齐，是无效的
    }

    uint32_t page = paddr / PAGE_SIZE;
    // 禁止释放低1MB和超出RAM范围的页
    if (page < (LOW_MEMORY_SIZE / PAGE_SIZE) || page >= pmm_max_ram_page)
        return;

    // 尝试释放一个已经是空闲的页
    if (!pmm_test_bit(page))
        return;

    pmm_clear_bit(page); // 标记为空闲
    pmm_free_pages++;
}

uint32_t pmm_get_free_page_count(void)
{
    return pmm_free_pages;
}
// ====================================================================
// 调试与转储函数
// ====================================================================

/* 辅助：打印一行 32 页 bitmap 的 ASCII 图例 */
static void dump_bitmap_line(uint32_t start_page, uint32_t end_page)
{
    char line[DUMP_LINE_BUFFER_SIZE];
    uint32_t idx = 0;
    for (uint32_t p = start_page; p < end_page && p < pmm_total_pages; ++p)
    {
        line[idx++] = pmm_test_bit(p) ? '#' : '.';
    }
    line[idx] = '\0';
    vga_printf("%x: %s\n", start_page * PAGE_SIZE, line);
}

/* 主转储函数 */
void pmm_dump(void)
{
    vga_printf("========== PMM dump ==========\n");
    vga_printf("total_pages = %d  (%d MiB)\n",
               pmm_total_pages, (pmm_total_pages * PAGE_SIZE) / MIB);
    vga_printf("free_pages  = %d  (%d MiB)\n",
               pmm_free_pages, (pmm_free_pages * PAGE_SIZE) / MIB);
    vga_printf("used_pages  = %d  (%d MiB)\n",
               pmm_total_pages - pmm_free_pages,
               ((pmm_total_pages - pmm_free_pages) * PAGE_SIZE) / MIB);
    vga_printf("bitmap size = %d bytes\n", pmm_bitmap_size_bytes);
    vga_printf("------------------------------\n");

    for (uint32_t p = 0; p < pmm_total_pages; p += 32)
    {
        dump_bitmap_line(p, p + 32);
    }
    vga_printf("==========  end dump  =========\n");
}

/**
 * @brief 通用的区域转储辅助函数
 * @param dump_used_regions true: 打印已用区; false: 打印空闲区
 */
static void pmm_dump_regions(bool_t dump_used_regions)
{
    uint32_t p = 0;
    uint32_t run_start = 0;
    bool_t in_run = false;
    const char *region_type_str = dump_used_regions ? "" : "[FREE] ";

    vga_printf("==== PMM %sregions ====\n", dump_used_regions ? "used " : "free ");

    uint32_t scan_limit = dump_used_regions ? pmm_total_pages : pmm_max_ram_page;

    while (p < scan_limit)
    {
        bool_t used = pmm_test_bit(p);
        bool_t is_target_region = (dump_used_regions && used) || (!dump_used_regions && !used);

        if (is_target_region && !in_run)
        {
            run_start = p;
            in_run = true;
        }
        else if (!is_target_region && in_run)
        {
            uint32_t run_end = p - 1;
            uint32_t pages = run_end - run_start + 1;
            uint32_t bytes = pages * PAGE_SIZE;

            vga_printf("%s%x -- %x  %d KiB",
                       region_type_str,
                       run_start * PAGE_SIZE,
                       (run_end + 1) * PAGE_SIZE - 1,
                       bytes / KIB);
            if (bytes >= MIB)
                vga_printf("  (%d MiB)", bytes / MIB);
            vga_printf("\n");

            in_run = false;
        }
        ++p;
    }

    if (in_run)
    {
        uint32_t pages = p - run_start;
        uint32_t bytes = pages * PAGE_SIZE;
        vga_printf("%s%x -- %x  %d KiB",
                   region_type_str,
                   run_start * PAGE_SIZE,
                   (p * PAGE_SIZE) - 1,
                   bytes / KIB);
        if (bytes >= MIB)
            vga_printf("  (%d MiB)", bytes / MIB);
        vga_printf("\n");
    }
    vga_printf("==== end of %s ====\n", dump_used_regions ? "used " : "free ");
}

void pmm_dump_used(void)
{
    pmm_dump_regions(true);
}

void pmm_dump_free(void)
{
    pmm_dump_regions(false);
}