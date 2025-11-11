[bits 16]
org 0x8000

%include "../common/bootmacros.inc"

_start:
    jmp loader_start

; 定义 GDT
DEFINE_STANDARD_GDT

loader_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    SET_STACK_AT_MBR

    call cls
    mov bx, MSG_REAL_MODE
    call print_bios

    ; 启用A20地址线
    in al, 0x92
    or al, 0000_0010b
    out 0x92, al

    ; 加载GDT并进入保护模式
    lgdt [gdt_descriptor]
    ; 设置 PE
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; 远跳刷新CS
    jmp SELECTOR_CODE32:init_pm

; ------------------------------
; BIOS文本输出
; ------------------------------
print_bios:
    pusha
.loop:
    mov al, [bx]
    test al, al
    jz .done
    mov ah, 0x0e
    int 0x10
    inc bx
    jmp .loop
.done:
    popa
    ret

cls:
    mov ax, 0x0600
    mov bx, 0x0700
    mov cx, 0
    mov dx, 184Fh
    int 0x10
    mov ah, 0x02
    xor dx, dx
    xor bh, bh
    int 0x10
    ret

MSG_REAL_MODE db "Started in 16-bit real mode (BIOS)",0
MSG_PROT_MODE db "Now in 32-bit protected mode (direct video)",0
MSG_ENABLE_PAGE db "Now Enable Page",0
; ------------------------------
; 保护模式初始化
; ------------------------------
[bits 32]

%include "../common/lib/pm_utils.S"

init_pm:
    mov ax, SELECTOR_DATA32
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov ss, ax
    mov ax, SELECTOR_VIDEO
    mov gs, ax

    ; 切换栈
    SET_STACK_AT_PM

    ; 清理屏幕
    call clear_screen

    ; 打印保护模式信息
    push 0x0F
    push MSG_PROT_MODE
    call println
    add esp, 8

    call setup_page

    ; 打印开启分页信息
    push 0x0F
    push MSG_ENABLE_PAGE
    call println
    add esp, 8

    cli
.hang:
    hlt
    jmp .hang

setup_page:
    ; 清除页目录的内存空间
    push PAGE_SIZE                    ; n
    push 0                            ; val = 0
    push PAGE_DIR_PHYISCAL_ADDR       ; dest
    call memset
    add esp, 12
.create_pde:
    ; first pde
    mov eax, PAGE_DIR_PHYISCAL_ADDR - PAGE_SIZE
    or eax, PTE_USER | PTE_WRITE | PTE_PRESENT
    mov [PAGE_DIR_PHYISCAL_ADDR + 0], eax
    mov [PAGE_DIR_PHYISCAL_ADDR + 768 * 4], eax

    ; the second pde - we use this 4MB virtual space for all kernel page tales :)
    mov eax, PAGE_DIR_PHYISCAL_ADDR
    or eax, PTE_USER | PTE_WRITE | PTE_PRESENT
    mov [PAGE_DIR_PHYISCAL_ADDR + 769 * 4], eax

    ; other kernel pde
    mov eax, PAGE_DIR_PHYISCAL_ADDR + PAGE_SIZE
    or eax, PTE_USER | PTE_WRITE | PTE_PRESENT
    mov ecx, 254
    mov edx, PAGE_DIR_PHYISCAL_ADDR + 770 * 4
.create_kernel_pde:
    mov [edx], eax
    add eax, PAGE_SIZE
    add edx, 4
    loop .create_kernel_pde

    ; create the first page table, for 1MB low memory
    mov eax, 0
    or eax, PTE_USER | PTE_WRITE | PTE_PRESENT
    mov ecx, 256
    mov edx, PAGE_DIR_PHYISCAL_ADDR - PAGE_SIZE
.create_pte:
    mov [edx], eax
    add eax, PAGE_SIZE
    add edx, 4
    loop .create_pte

    call enable_page
    ret

; 启用分页，并调整段寄存器与 GDT 到高地址空间
enable_page:
    sgdt [gdt_descriptor]

    ; 调整视频段描述符的基址
    ; +2 表示描述符表中的 base 偏移
    mov ebx, [gdt_descriptor + 2]
    ; 0x18 表示第三个GDT, 也就是视频段
    ; +4 是为了修改到基址的高位部分
    or dword [ebx + 0x18 + 4], KERNEL_SPACE_START

    ; 移动 GDT 到大于 KERNEL_SPACE_START 的位置
    ; +2 表示描述符表中的 base 偏移
    add dword [gdt_descriptor + 2], KERNEL_SPACE_START

    ; 将栈移动到内核空间 KERNEL_SPACE_START
    mov eax, [esp]
    add esp, KERNEL_SPACE_START
    mov [esp], eax

    ; 设置页目录地址到 CR3 寄存器
    mov eax, PAGE_DIR_PHYISCAL_ADDR
    mov cr3, eax
    
    ; 启动分页
    mov eax, cr0
    or  eax, CR0_PG
    mov cr0, eax

    ; 在分页模式下重新加载 GDT
    lgdt [gdt_descriptor]

    ; 在分页模式下重新设置视频段
    mov ax, SELECTOR_VIDEO
    mov gs, ax

    ret

