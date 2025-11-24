/**
 * @file pmm.h
 * @brief 物理内存管理器 公共接口头文件
 *
 * 本文件定义了物理内存管理器的抽象接口。
 * 内核的其他所有部分都应通过此接口与物理内存管理器交互，
 * 而不应关心其底层实现（例如位图、伙伴系统等）。
 * 这种设计使得底层实现可以被独立地替换或优化，而不会影响到上层代码。
 */

#ifndef PMM_H
#define PMM_H

#include "boot_info.h"
#include "types.h"

/**
 * @brief 物理页大小，定义为 4KB
 */
#define PAGE_SIZE 4096

/**
 * @brief 物理页大小的对数（以2为底），2^12 = 4096
 */
#define PAGE_SHIFT 12

/**
 * @brief 调试用的字符串行缓存
 */
#define DUMP_LINE_BUFFER_SIZE (64)

/**
 * @brief 将地址向下对齐到页边界
 * @param addr 输入地址
 * @return 向下对齐后的地址
 */
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(PAGE_SIZE - 1))

/**
 * @brief 将地址向上对齐到页边界
 * @param addr 输入地址
 * @return 向上对齐后的地址
 */
#define PAGE_ALIGN_UP(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

/**
 * @brief 直接映射区的虚拟基地址
 * @note
 * 此宏定义了物理地址到虚拟地址的线性映射区域的起始地址。
 * 例如，物理地址 0x0 会被映射到虚拟地址 0xC0000000。
 * 这个值必须与你的页表设置和链接脚本保持一致。
 */
#define DIRECT_MAP_BASE 0xC0000000

/**
 * @brief 将物理地址转换为虚拟地址
 * @param paddr 物理地址
 * @return 对应的虚拟地址
 */
#define P2V(paddr) ((uintptr_t)(paddr) + DIRECT_MAP_BASE)

/**
 * @brief 将虚拟地址转换为物理地址
 * @param vaddr 虚拟地址
 * @return 对应的物理地址
 * @note 此宏仅适用于在直接映射区内的虚拟地址
 */
#define V2P(vaddr) ((uintptr_t)(vaddr) - DIRECT_MAP_BASE)

// ====================================================================
// PMM 核心接口
// ====================================================================

/**
 * @brief 初始化物理内存管理器
 *
 * 此函数是 PMM 的入口点，它必须且只能在系统启动初期被调用一次。
 * 它会解析引导加载程序提供的内存信息，建立内部数据结构，
 * 并准备好用于分配和释放物理内存。
 *
 * @param boot_info 指向由引导加载程序填充的启动信息结构体的指针。
 *                  该结构体必须包含 E820 内存映射和内核段信息。
 */
void pmm_init(boot_info_t *boot_info);

/**
 * @brief 分配一个物理页
 *
 * 从可用的物理内存中分配一个连续的、大小为 PAGE_SIZE 的物理页。
 * 使用 Next Fit 策略以提高分配效率。
 *
 * @return 成功时返回已分配物理页的起始物理地址（按页对齐）。
 *         如果内存耗尽或分配失败，则返回 0。
 * @note 调用者在使用完该页后，必须调用 pmm_free_page 来释放它。
 */
uint32_t pmm_alloc_page(void);

/**
 * @brief 释放一个先前分配的物理页
 *
 * 将一个由 pmm_alloc_page 分配的物理页归还给内存管理器，
 * 使其可以被后续的分配请求重新使用。
 *
 * @param paddr 要释放的物理页的起始物理地址。
 *              该地址必须是之前由 pmm_alloc_page 返回的有效地址。
 * @note 释放一个未分配的页、一个无效地址或一个已经被释放的页，
 *       其行为是未定义的，可能会导致系统不稳定。
 */
void pmm_free_page(uint32_t paddr);

/**
 * @brief 获取当前空闲物理页的数量
 *
 * @return 当前可供分配的、大小为 PAGE_SIZE 的空闲物理页的总数。
 * @note 此函数可用于监控内存使用情况或进行调试。
 */
uint32_t pmm_get_free_page_count(void);

// ====================================================================
// PMM 调试与状态接口
// ====================================================================

/**
 * @brief 打印完整的物理内存位图信息
 *
 * 此函数会以 ASCII 图表的形式打印出整个物理地址空间的位图状态，
 * 并显示总页数、空闲页数等统计信息。主要用于深度调试。
 */
void pmm_dump(void);

/**
 * @brief 打印所有连续的、已使用的物理内存区域
 *
 * 此函数会遍历位图，找出所有连续的已使用内存块，并打印它们的
 * 起始地址、结束地址和大小。这有助于了解哪些区域被内核或硬件占用。
 */
void pmm_dump_used(void);

/**
 * @brief 打印所有连续的、可分配的空闲物理内存区域
 *
 * 此函数会遍历位图，只在有效的 RAM 范围内，找出所有连续的空闲内存块，
 * 并打印它们的起始地址、结束地址和大小。这是评估可用内存的主要方式。
 */
void pmm_dump_free(void);

#endif // PMM_H