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

    cli
.hang:
    hlt
    jmp .hang