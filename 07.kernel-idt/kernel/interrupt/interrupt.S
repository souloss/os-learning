; ============================================================
; interrupt.S — x86 32-bit Interrupt and IRQ Handler Stubs
; 无循环、清晰易懂、功能完整
; ============================================================

[BITS 32]

%define KERNEL_DS 0x10        ; 内核数据段选择子

; 外部C函数
extern isr_handler
extern irq_handler

; ------------------------------------------------------------
; 宏定义（简单、无循环展开）
; ------------------------------------------------------------

; --- 无错误码的CPU异常 ---
%macro ISR_NOERR 1
global isr%1
isr%1:
    push dword 0              ; 错误码占位
    push dword %1             ; 中断号
    jmp isr_common_stub
%endmacro

; --- 带错误码的CPU异常 ---
%macro ISR_ERR 1
global isr%1
isr%1:
    push dword %1             ; 中断号（CPU已自动推送错误码）
    jmp isr_common_stub
%endmacro

; --- 外部硬件中断 IRQ ---
%macro IRQ 2
global irq%1
irq%1:
    push dword %2             ; 中断号 = IRQ 基号 + 偏移
    jmp irq_common_stub
%endmacro


; ------------------------------------------------------------
; === 定义 CPU 异常（ISR 0–31） ===
; ------------------------------------------------------------
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31


; ------------------------------------------------------------
; === 定义外部硬件中断（IRQ 0–15） ===
; ------------------------------------------------------------
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47


; ------------------------------------------------------------
; === 通用 ISR 处理入口 ===
; ------------------------------------------------------------
isr_common_stub:
    pusha                     ; 保存所有通用寄存器

    push ds
    push es
    push fs
    push gs

    mov ax, KERNEL_DS         ; 加载内核数据段
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp                  ; 将当前栈顶（即frame的起始地址）压栈
    call isr_handler          ; 调用C层处理
    add esp, 4                ; 弹出参数

    pop gs
    pop fs
    pop es
    pop ds

    popa
    add esp, 8                ; 弹出错误码和中断号
    iret


; ------------------------------------------------------------
; === 通用 IRQ 处理入口 ===
; ------------------------------------------------------------
irq_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, KERNEL_DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa

    add esp, 4                ; 弹出中断号
    iret