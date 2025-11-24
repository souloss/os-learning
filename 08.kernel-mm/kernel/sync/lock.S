global atomic_exchange
global compare_and_exchange
global atomic_fetch_add
global atomic_inc
global atomic_dec
global barrier
global mb
global rmb
global wmb
global get_eflags
global set_eflags
global cpu_cli
global cpu_sti
global cpu_save_flags_and_cli

section .text

; ----------------------------------------------------------
; uint32_t atomic_exchange(volatile uint32_t *ptr, uint32_t val);
; 返回旧值
; ----------------------------------------------------------
atomic_exchange:
    mov     ecx, [esp+4]    ; ptr
    mov     eax, [esp+8]    ; val
    xchg    [ecx], eax      ; 原子交换
    ret

; ----------------------------------------------------------
; uint32_t compare_and_exchange(volatile uint32_t *ptr,
;                                uint32_t expected,
;                                uint32_t newval);
; 成功返回 1，失败返回 0
; ----------------------------------------------------------
compare_and_exchange:
    push    ebx
    mov     edx, [esp+8]    ; ptr
    mov     eax, [esp+12]   ; expected
    mov     ebx, [esp+16]   ; newval
    lock cmpxchg [edx], ebx
    sete    al              ; 成功时 al=1
    movzx   eax, al
    pop     ebx
    ret

; ----------------------------------------------------------
; uint32_t atomic_fetch_add(volatile uint32_t *ptr, int32_t addend);
; 返回旧值
; ----------------------------------------------------------
atomic_fetch_add:
    mov     edx, [esp+4]    ; ptr
    mov     eax, [esp+8]    ; addend
    lock xadd [edx], eax    ; eax = old
    ret

; ----------------------------------------------------------
; uint32_t atomic_inc(volatile uint32_t *ptr);
; 返回新值
; ----------------------------------------------------------
atomic_inc:
    mov     edx, [esp+4]
    mov     eax, 1
    lock xadd [edx], eax
    inc     eax             ; 返回新值
    ret

; ----------------------------------------------------------
; uint32_t atomic_dec(volatile uint32_t *ptr);
; 返回新值
; ----------------------------------------------------------
atomic_dec:
    mov     edx, [esp+4]
    mov     eax, -1
    lock xadd [edx], eax
    dec     eax
    ret

; ----------------------------------------------------------
; void barrier(void);
; 全屏障（带 lock 前缀）
; ----------------------------------------------------------
barrier:
    lock or dword [esp], 0
    ret

; ----------------------------------------------------------
; void mb(void);
; 内存屏障
; ----------------------------------------------------------
mb:
    lock add dword [esp], 0
    ret

; ----------------------------------------------------------
; void rmb(void);
; 读屏障（lfence）
; ----------------------------------------------------------
rmb:
    db 0x0F, 0xAE, 0xE8   ; lfence
    ret

; ----------------------------------------------------------
; void wmb(void);
; 写屏障（sfence）
; ----------------------------------------------------------
wmb:
    db 0x0F, 0xAE, 0xF8   ; sfence
    ret

; ----------------------------------------------------------
; uint32_t get_eflags(void);
; ----------------------------------------------------------
get_eflags:
    pushfd
    pop     eax
    ret

; ----------------------------------------------------------
; void set_eflags(uint32_t eflags);
; ----------------------------------------------------------
set_eflags:
    mov     eax, [esp+4]
    push    eax
    popfd
    ret

; ----------------------------------------------------------
; void cpu_cli(void);
; ----------------------------------------------------------
cpu_cli:
    cli
    ret

; ----------------------------------------------------------
; void cpu_sti(void);
; ----------------------------------------------------------
cpu_sti:
    sti
    ret

; ----------------------------------------------------------
; uint32_t cpu_save_flags_and_cli(void);
; 返回关中断前的 eflags
; ----------------------------------------------------------
cpu_save_flags_and_cli:
    pushfd
    pop     eax
    cli
    ret