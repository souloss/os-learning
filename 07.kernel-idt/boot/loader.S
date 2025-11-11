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

    call init_kernel

    cli
.hang:
    hlt
    jmp .hang

; ------------------------------
; 内存分页
; ------------------------------
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

set_pages_mapping:
  push ebp
  mov ebp, esp
  push esi
  push edi
  push ebx

  mov esi, [ebp + 8]    ; virtual addr start
  mov edi, [ebp + 12]   ; physical addr start
  mov ecx, [ebp + 16]   ; pages

  shr esi, 12           ; page table index
  or edi, PTE_USER | PTE_WRITE | PTE_PRESENT
.map_next_page:
  mov [PAGE_TABLES_VIRTUAL_ADDR_START + esi * 4], edi
  add esi, 1
  add edi, PAGE_SIZE
  loop .map_next_page

  pop ebx
  pop edi
  pop esi
  pop ebp
  ret

clear_pages_mapping:
  push ebp
  mov ebp, esp
  push esi
  push ebx

  mov esi, [ebp + 8]    ; virtual addr start
  mov ecx, [ebp + 12]   ; pages

  shr esi, 12  ; page table index
.clear_next_page_map:
  mov dword [PAGE_TABLES_VIRTUAL_ADDR_START + esi * 4], 0
  add esi, 1
  add edi, PAGE_SIZE
  loop .clear_next_page_map

  pop ebx
  pop esi
  pop ebp
  ret

; ------------------------------
; 内核初始化
; ------------------------------
init_kernel:
  call allocate_pages_for_kernel
  call load_hd_kernel_image
  call do_load_kernel
  
  ; init floating point unit before entering the kernel
  finit

  ; move stack to 0xF0000000
  mov esp, KERNEL_STACK_TOP - 16
  mov ebp, esp

  ; let's jump to kernel entry :)
  jmp eax
  ret

allocate_pages_for_kernel:
  ; allocate pages to load kernel bin image
  mov ecx, KERNEL_BIN_MAX_SIZE
  shr ecx, 12
  push ecx                            ; arg 3: num of pages to load kernel bin image
  push KERNEL_BIN_LOAD_PHYSICAL_ADDR  ; arg 2: physical addr
  push KERNEL_BIN_LOAD_VIRTUAL_ADDR   ; arg 1: virtual addr
  call set_pages_mapping
  add esp, 12

  ; allocate pages to place kernel sections
  mov ecx, KERNEL_MEM_MAX_SIZE
  shr ecx, 12
  push ecx                         ; arg 3: num of pages to place kernel
  push KERNEL_PHYSICAL_ADDR_START  ; arg 2: physical addr
  push KERNEL_VIRTUAL_ADDR_START   ; arg 1: virtual addr
  call set_pages_mapping
  add esp, 12

  ; allocate one page for kernel stack
  mov ecx, 2
  push ecx  ; arg 3: num of pages
  push KERNEL_STACK_PHYSICAL_ADDR        ; arg 2: physical addr
  push KERNEL_STACK_TOP - PAGE_SIZE      ; arg 1: virtual addr
  call set_pages_mapping
  add esp, 12

  ret

load_hd_kernel_image:
  push KERNEL_BIN_LOAD_VIRTUAL_ADDR  ; buffer
  push KERNEL_SECTORS                ; sector count
  push KERNEL_START_SECTOR           ; LBA
  call read_disk
  add esp, 12
  ret

; load kernel sections to memory
; return the kernel entry point address
do_load_kernel:
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

    ; 获取程序头表偏移 (e_phoff)
    mov ebx, [KERNEL_BIN_LOAD_VIRTUAL_ADDR + ELF_E_PHOFF]
    add ebx, KERNEL_BIN_LOAD_VIRTUAL_ADDR   ; EBX = 程序头表的实际地址

    ; 获取程序头数量 (e_phnum)
    mov cx, [KERNEL_BIN_LOAD_VIRTUAL_ADDR + ELF_E_PHNUM]

    ; 获取每个程序头大小 (e_phentsize)
    movzx edx, word [KERNEL_BIN_LOAD_VIRTUAL_ADDR + ELF_E_PHENTSIZE]

.load_each_segment:
    ; 加载 PT_LOAD 类型的段
    mov eax, [ebx + PH_TYPE]
    cmp eax, 1                  ; PT_LOAD ?
    jne .next_program_header

    ; 调用 memcpy(p_vaddr, file_base + p_offset, p_filesz)
    push dword [ebx + PH_FILESZ]    ; 第三个参数：复制字节数
    mov eax, [ebx + PH_OFFSET]      ; 读取段在 ELF 文件内的偏移
    add eax, KERNEL_BIN_LOAD_VIRTUAL_ADDR ; 源地址 = 文件起始 + 偏移
    push eax                        ; 第二个参数：源地址
    push dword [ebx + PH_VADDR]     ; 第一个参数：目标虚拟地址

    call memcpy
    add esp, 12                     ; 清理参数栈

.next_program_header:
    add ebx, edx                    ; 指向下一个程序头
    loop .load_each_segment         ; CX-- 并判断是否结束

    ; 返回 ELF 文件入口点 (e_entry)
    mov eax, [KERNEL_BIN_LOAD_VIRTUAL_ADDR + ELF_E_ENTRY]
    ret