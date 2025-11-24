/**
 * @file vmm.h
 * @brief 虚拟内存管理器 公共接口头文件 (固定页表版)
 *
 * 本文件定义了虚拟内存管理器的抽象接口。
 * 它建立在由引导加载程序预先分配的固定页目录和页表之上。
 */

#ifndef VMM_H
#define VMM_H

#include "types.h"
#include "pmm.h"
#include "kernel.h"
#include "interrupt.h"

// ====================================================================
// 核心常量与宏定义
// ====================================================================

// 页面标志位
#define PAGE_PRESENT (1 << 0)
#define PAGE_RW (1 << 1)
#define PAGE_USER (1 << 2)
#define PAGE_WRITETHROUGH (1 << 3)
#define PAGE_CACHE_DISABLE (1 << 4)
#define PAGE_ACCESSED (1 << 5)
#define PAGE_DIRTY (1 << 6)

#define PAGE_KERNEL_FLAGS (PAGE_PRESENT | PAGE_RW)

// 地址对齐宏
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define DIRECT_MAP_BASE 0xC0000000
#define P2V(paddr) ((uintptr_t)(paddr) + DIRECT_MAP_BASE)
#define V2P(vaddr) ((uintptr_t)(vaddr) - DIRECT_MAP_BASE)

// ====================================================================
// 内存布局常量
// ====================================================================

// ********************* virtual memory layout *********************************
#define PAGE_DIR_VIRTUAL 0xC0701000         /**< 页目录的虚拟地址 */
#define PAGE_TABLES_VIRTUAL_ADDR 0xC0400000 /**< 页表区域的起始虚拟地址 */
#define KERNEL_LOAD_VIRTUAL_ADDR 0xC0800000 /**< 内核加载的虚拟地址 */

// ********************* physical memory layout *********************************
#define KERNEL_PAGE_DIR_PHY 0x00101000       /**< 页目录的物理地址 */
#define KERNEL_LOAD_PHYSICAL_ADDR 0x00200000 /**< 内核加载的物理地址 */

// ====================================================================
// 硬件相关的数据结构
// ====================================================================
typedef struct page_table_entry
{
  uint32_t present : 1;
  uint32_t rw : 1;
  uint32_t user : 1;
  uint32_t writethrough : 1;
  uint32_t cache_disable : 1;
  uint32_t accessed : 1;
  uint32_t dirty : 1;
  uint32_t pat : 1;
  uint32_t global : 1;
  uint32_t avail : 3;
  uint32_t frame_addr : 20;
} __attribute__((packed)) page_table_entry_t;

typedef page_table_entry_t page_directory_entry_t;

typedef struct page_directory
{
  page_directory_entry_t entries[1024];
} __attribute__((aligned(PAGE_SIZE))) page_directory_t;

typedef struct page_table
{
  page_table_entry_t entries[1024];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;

// ====================================================================
// VMM 核心接口
// ====================================================================

/**
 * @brief 初始化虚拟内存管理器
 *
 * 此函数在 PMM 初始化之后、由加载程序开启分页之后调用。
 * 它的作用是同步 C 代码中的 VMM 状态与加载程序已经建立好的页表环境。
 * 它不会再次开启分页。
 */
void vmm_init(void);

/**
 * @brief 将一个虚拟地址映射到一个物理地址
 *
 * @param virt_addr 要映射的虚拟地址（必须按页对齐）
 * @param phys_addr 要映射的物理地址（必须按页对齐）
 * @param flags 页的权限标志 (如 PAGE_KERNEL_FLAGS)
 * @return true 映射成功
 * @return false 映射失败 (例如，页表未正确设置)
 */
bool_t vmm_map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);

/**
 * @brief 为一个虚拟地址分配一个物理页并映射
 *
 * 这是一个按需分页的辅助函数，它会先分配物理页，再进行映射。
 *
 * @param virt_addr 要映射的虚拟地址（必须按页对齐）
 * @param flags 页的权限标志
 * @return true 映射成功
 * @return false 映射失败 (通常是物理内存耗尽)
 */
bool_t vmm_alloc_and_map_page(uint32_t virt_addr, uint32_t flags);

/**
 * @brief 取消一个虚拟地址的映射
 *
 * @param virt_addr 要取消映射的虚拟地址（必须按页对齐）
 */
void vmm_unmap_page(uint32_t virt_addr);

/**
 * @brief 获取一个虚拟地址对应的物理地址
 *
 * @param virt_addr 虚拟地址
 * @return uint32_t 对应的物理地址。如果未映射，则返回 0。
 */
uint32_t vmm_get_phys_addr(uint32_t virt_addr);

/**
 * @brief 切换到新的页目录（用于进程切换）
 *
 * @note 此功能需要自映射页表的支持，当前为简化实现。
 * @param new_directory_phys_addr 新页目录的物理地址
 */
void vmm_switch_page_directory(uint32_t new_directory_phys_addr);

/**
 * @brief 获取当前页目录的物理地址
 *
 * @return uint32_t 当前页目录的物理地址
 */
uint32_t vmm_get_current_directory_phys_addr(void);

/**
 * @brief Page Fault (中断 14) 的处理程序
 *
 * 当 CPU 访问一个未映射的页面时，会调用此函数。
 * 本实现采用按需分页策略，为故障地址分配物理页并建立映射。
 *
 * @param frame 指向中断发生时 CPU 上下文的指针
 */
void vmm_page_fault_handler(interrupt_frame_t *frame);

#endif // VMM_H