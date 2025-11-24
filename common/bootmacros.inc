; -----------------------------------------------------------------------
; bootmacros.inc — Boot constants, GDT macros, selectors, and paging helpers
; NASM syntax
; -----------------------------------------------------------------------

; =================================================================================
; 内存布局常量
; =================================================================================
MBR_BASE_ADDR        equ 0x7C00      ; BIOS加载MBR的物理地址
LOADER_BASE_ADDR     equ 0x8000      ; 引导程序加载地址
LOADER_START_SECTOR  equ 1           ; 引导程序起始扇区
E820_BUFFER          equ 0x9000      ; E820 内存映射表缓冲区地址
E820_SIGNATURE       equ 0x534D4150  ; ASCII "SMAP"
BOOT_INFO_ADDR       equ 0x0A000     ; 引导信息结构体地址
; =================================================================================
; 视频内存常量
; =================================================================================
VIDEO_MEMORY         equ 0xB8000     ; 文本模式视频内存地址
WHITE_ON_BLACK       equ 0x0F        ; 黑底白字属性字节

;=================================================================================
; CR0 位掩码 
; =================================================================================
CR0_PE          equ 1 << 0      ; Protection Enable
CR0_MP          equ 1 << 1      ; Monitor Coprocessor
CR0_EM          equ 1 << 2      ; Emulation
CR0_TS          equ 1 << 3      ; Task Switched
CR0_ET          equ 1 << 4      ; Extension Type
CR0_NE          equ 1 << 5      ; Numeric Error
CR0_WP          equ 1 << 16     ; Write Protect
CR0_AM          equ 1 << 18     ; Alignment Mask
CR0_NW          equ 1 << 29     ; Not Write-through
CR0_CD          equ 1 << 30     ; Cache Disable
CR0_PG          equ 1 << 31     ; PaGing
; ================================================================================
; GDT 结构宏定义
; ================================================================================
; 每个 GDT 描述符占 8 字节，总结构如下（按低地址到高地址）：
;
;  31                                16 15                                   0
;  +------------------------------------+------------------------------------+
;  |         Base[15:0]                 |         Limit[15:0]                |
;  +------------------------------------+------------------------------------+
;  |  Base[23:16]  | Access Byte        | Flags | Limit[19:16] | Base[31:24] |
;  +------------------------------------+------------------------------------+
;
; Access Byte （第 5 字节）控制段的类型、存在、特权等；
; Flags （第 6 字节高 4 位）控制段粒度、操作数大小、64 位模式等。
;
; ------------------------------------------------------------------------------
; Intel 手册：Descriptor Type Fields
;   Access Byte (bits 8..15)    → 控制“段类型”
;   Flags (bits 20..23)         → 控制“段属性”
; ===============================================================================


; ================================================================================
; [1] Access Byte 位布局（8 bits）
; ================================================================================
; bit | 名称 | 说明
; ----+------+--------------------------------------------------------------
;  7  | P    | 段存在位 (1=有效)
; 6-5 | DPL  | 特权级别（0=最高, 3=最低）
;  4  | S    | 描述符类型 (1=代码/数据, 0=系统段)
; 3-0 | TYPE | 段类型（取决于S=1或S=0）

; ------------------------------------------------------------------------------
; [Access Byte] 位定义（保持互斥且可组合）
; ------------------------------------------------------------------------------

; (bit 7)
ACCESS_P          equ 0x80        ; P=1, 段存在于内存

; (bits 6-5) 特权级（DPL）
ACCESS_DPL0       equ 0x00        ; CPL=0（内核）
ACCESS_DPL1       equ 0x20        ; CPL=1
ACCESS_DPL2       equ 0x40        ; CPL=2
ACCESS_DPL3       equ 0x60        ; CPL=3（用户）

; (bit 4)
ACCESS_S_CODEDATA equ 0x10        ; S=1, 普通代码/数据段
ACCESS_S_SYSTEM   equ 0x00        ; S=0, 系统段 (TSS, LDT等)

; (bits 3-0) TYPE 字段
; --- 数据段类型（S=1 且 TYPE[3]=0）---
ACCESS_DATA_RD    equ 0x00        ; 只读
ACCESS_DATA_RDA   equ 0x01        ; 只读, 已访问
ACCESS_DATA_RW    equ 0x02        ; 可读写
ACCESS_DATA_RWA   equ 0x03        ; 可读写, 已访问
ACCESS_DATA_EXP   equ 0x04        ; 向下扩展，只读
ACCESS_DATA_EXPW  equ 0x06        ; 向下扩展，可写

; --- 代码段类型（S=1 且 TYPE[3]=1）---
ACCESS_CODE_X     equ 0x08        ; 只执行
ACCESS_CODE_XR    equ 0x0A        ; 可执行+可读（最常用）
ACCESS_CODE_XC    equ 0x0C        ; 一致代码段（低特权可执行）
ACCESS_CODE_XRC   equ 0x0E        ; 可执行+可读+一致

; ------------------------------------------------------------------------------
; 常用组合（可直接使用）
; ------------------------------------------------------------------------------
ACCESS_CODE32_KERNEL equ ACCESS_P | ACCESS_DPL0 | ACCESS_S_CODEDATA | ACCESS_CODE_XR
ACCESS_DATA32_KERNEL equ ACCESS_P | ACCESS_DPL0 | ACCESS_S_CODEDATA | ACCESS_DATA_RW
ACCESS_CODE32_USER   equ ACCESS_P | ACCESS_DPL3 | ACCESS_S_CODEDATA | ACCESS_CODE_XR
ACCESS_DATA32_USER   equ ACCESS_P | ACCESS_DPL3 | ACCESS_S_CODEDATA | ACCESS_DATA_RW


; ================================================================================
; [2] Flags 位布局（高 4 位 bits 20..23）
; ================================================================================
; bit | 名称 | 说明
; ----+------+--------------------------------------------------------------
;  7-4| Flags高4位  | G / D / L / AVL
;  3-0| Limit高4位  | 段界限高4位（由 GDT_ENTRY 宏补充）
;
; Flags字段高4位（即 descriptor 第6字节的高半字节）：
;    7 6 5 4
;    G D L AVL
;
; ------------------------------------------------------------------------------
; [Flags] 位定义（高4位部分）
; ------------------------------------------------------------------------------

FLAGS_G_4K      equ 0x8     ; G=1, 粒度=4KB
FLAGS_G_BYTE    equ 0x0     ; G=0, 粒度=1B

FLAGS_D_32      equ 0x4     ; D=1, 32位默认操作数大小
FLAGS_D_16      equ 0x0     ; D=0, 16位默认操作数大小

FLAGS_L_64      equ 0x2     ; L=1, 64位代码段（仅 IA-32e 模式）
FLAGS_L_32      equ 0x0     ; L=0, 32位段

FLAGS_AVL       equ 0x1     ; AVL=1, 软件可用位（一般置0）

; ------------------------------------------------------------------------------
; 常用组合
; ------------------------------------------------------------------------------
FLAGS_4K_32BIT  equ FLAGS_G_4K | FLAGS_D_32 | FLAGS_L_32
FLAGS_4K_64BIT  equ FLAGS_G_4K | FLAGS_L_64
FLAGS_BYTE_16BIT equ FLAGS_G_BYTE | FLAGS_D_16


; ================================================================================
; [3] 通用常量
; ================================================================================
DESC_BASE_ZERO   equ 0x00000000
DESC_LIMIT_MAX   equ 0xFFFFF     ; 当 G=4K 时, 表示 4GB 空间


; ================================================================================
; [4] GDT Entry 构造宏
; ================================================================================
; GDT_ENTRY base, limit, access_byte, flags_nibble
; 输出: 共6条指令，生成完整8字节描述符
; ------------------------------------------------------------------------------
%macro GDT_ENTRY 4
    dw (%2 & 0xFFFF)                                 ; Limit[15:0]
    dw (%1 & 0xFFFF)                                 ; Base[15:0]
    db ((%1 >> 16) & 0xFF)                           ; Base[23:16]
    db %3                                            ; Access Byte
    db (((%2 >> 16) & 0x0F) | ((%4 & 0x0F) << 4))    ; Flags高4位 | Limit高4位
    db ((%1 >> 24) & 0xFF)                           ; Base[31:24]
%endmacro


; ================================================================================
; [5] 标准 GDT 定义（最小可运行）
; ================================================================================
%macro DEFINE_STANDARD_GDT 0
gdt_start:
    ; Null Descriptor（必须存在）
    GDT_ENTRY 0, 0, 0, 0

    ; 内核代码段: base=0, limit=4GB, 32位, 粒度4K
    GDT_ENTRY DESC_BASE_ZERO, DESC_LIMIT_MAX, ACCESS_CODE32_KERNEL, FLAGS_4K_32BIT

    ; 内核数据段
    GDT_ENTRY DESC_BASE_ZERO, DESC_LIMIT_MAX, ACCESS_DATA32_KERNEL, FLAGS_4K_32BIT

    ; 视频段 (0xB8000, limit约4KB)
    GDT_ENTRY 0x000B8000, 0x07FF, ACCESS_DATA32_KERNEL, FLAGS_G_BYTE

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
%endmacro


; ================================================================================
; [6] 段选择子定义
; ================================================================================
RPL0    equ 0
RPL3    equ 3
TI_GDT  equ 0
TI_LDT  equ 4

SELECTOR_NULL     equ (0 << 3) + TI_GDT + RPL0
SELECTOR_CODE32   equ (1 << 3) + TI_GDT + RPL0
SELECTOR_DATA32   equ (2 << 3) + TI_GDT + RPL0
SELECTOR_VIDEO    equ (3 << 3) + TI_GDT + RPL0
SELECTOR_UCODE32  equ (4 << 3) + TI_GDT + RPL3
SELECTOR_UDATA32  equ (5 << 3) + TI_GDT + RPL3

; =================================================================================
; 栈设置宏
; =================================================================================
; 将栈指针设置到MBR_BASE_ADDR
; 注意: 只有512字节的栈空间，使用时需谨慎
;
; 使用示例:
;   SET_STACK_AT_MBR
%macro SET_STACK_AT_MBR 0
    xor ax, ax
    mov ss, ax
    mov sp, MBR_BASE_ADDR
%endmacro

%macro SET_STACK_AT_PM 0
    mov esp, 0x00090000
    mov ebp, esp
%endmacro

; =================================================================================
; ATA 相关
; =================================================================================

ATA_PRIMARY_BASE      equ 0x1F0
ATA_REG_DATA          equ ATA_PRIMARY_BASE + 0
ATA_REG_SECCOUNT      equ ATA_PRIMARY_BASE + 2
ATA_REG_LBA0          equ ATA_PRIMARY_BASE + 3
ATA_REG_LBA1          equ ATA_PRIMARY_BASE + 4
ATA_REG_LBA2          equ ATA_PRIMARY_BASE + 5
ATA_REG_DRIVE         equ ATA_PRIMARY_BASE + 6
ATA_REG_STATUS_CMD    equ ATA_PRIMARY_BASE + 7

; =================================================================================
; 分页相关常量
; =================================================================================
PAGE_SIZE         equ 0x1000       ; 标准页大小 (4KB)
PAGE_ENTRY_SIZE   equ 8            ; 页表条目大小 (8字节)

PAGE_DIR_PHYISCAL_ADDR            equ  0x100000 + PAGE_SIZE
PAGE_DIR_VIRTUAL_ADDR             equ  0xC0701000
PAGE_TABLES_VIRTUAL_ADDR_START    equ  0xC0400000

; 页表条目标志位
PTE_PRESENT       equ 1 << 0       ; 存在位
PTE_WRITE         equ 1 << 1       ; 写允许位
PTE_USER          equ 1 << 2       ; 用户访问位
PTE_PWT           equ 1 << 3       ; 页级写通过
PTE_PCD           equ 1 << 4       ; 页级缓存禁用
PTE_ACCESSED      equ 1 << 5       ; 访问位
PTE_DIRTY         equ 1 << 6       ; 脏位
PTE_PAT           equ 1 << 7       ; 页属性表
PTE_GLOBAL        equ 1 << 8       ; 全局页
PTE_AVAIL1        equ 1 << 9       ; 可用位1
PTE_AVAIL2        equ 1 << 10      ; 可用位2
PTE_AVAIL3        equ 1 << 11      ; 可用位3

; =================================================================================
; 内核相关常量
; =================================================================================
PHY_MEM_SIZE           equ 32 * 1024 * 1024 ; 物理内存大小 (32MB)
KERNEL_START_SECTOR    equ 9                ; 内核起始扇区
KERNEL_BIN_MAX_SIZE    equ 1024 * 1024      ; 内核磁盘中的最大大小 (1MB)
KERNEL_MEM_MAX_SIZE    equ 1024 * 1024      ; 内核内存中的最大大小 (1MB)
KERNEL_SECTORS         equ KERNEL_BIN_MAX_SIZE / 512  ; 内核占用的扇区数

KERNEL_BIN_LOAD_MEM_MAX         equ   0xFFFFFFFF
KERNEL_BIN_LOAD_VIRTUAL_ADDR    equ   KERNEL_BIN_LOAD_MEM_MAX - KERNEL_BIN_MAX_SIZE + 1  ; 0xFFF00000
KERNEL_BIN_LOAD_PHYSICAL_ADDR   equ   PHY_MEM_SIZE - PAGE_SIZE - KERNEL_BIN_MAX_SIZE

KERNEL_SPACE_START              equ   0xC0000000

; 以 KERNEL_SPACE_START 为基址
; 第一个 4MB 保留，其中低 1MB 空间映射到了 physical 地址的低 1MB;
; 第二个 4MB 用来映射 kernel 的所有 page tables；
; 第三个 4MB，即 KERNEL_SPACE_START + 0x0800000 开始，作为加载、存放 kernel 代码和数据的空间
KERNEL_VIRTUAL_ADDR_START       equ   KERNEL_SPACE_START + 0x0800000
KERNEL_PHYSICAL_ADDR_START      equ   0x200000  ; 2MB

KERNEL_STACK_TOP                equ   0xF0000000
KERNEL_STACK_PHYSICAL_ADDR      equ   PHY_MEM_SIZE - 4 * PAGE_SIZE

; =================================================================================
; ELF 相关
; =================================================================================
; -----------------------------
; ELF Header 字段偏移 (单位: 字节)
; -----------------------------
ELF_E_ENTRY      equ        24    ; 程序入口点 e_entry
ELF_E_PHOFF      equ        28    ; 程序头表偏移 e_phoff
ELF_E_PHENTSIZE  equ        42    ; 每个程序头表项大小 e_phentsize
ELF_E_PHNUM      equ        44    ; 程序头表项数量 e_phnum

; -----------------------------
; Program Header (段头表) 字段偏移
; -----------------------------
PH_TYPE           equ       0    ; 段类型 p_type
PH_OFFSET         equ       4    ; 段在文件中的偏移 p_offset
PH_VADDR          equ       8    ; 段加载的虚拟地址 p_vaddr
PH_FILESZ         equ       16   ; 段文件中大小 p_filesz

; =================================================================================
; 结束
; =================================================================================