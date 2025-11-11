[bits 32]

global gdt_flush
gdt_flush:
    lgdt [eax]
    jmp 0x08:.reload_cs
.reload_cs:
    mov ax, 0x10 ;kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret


global tss_flush
tss_flush:
    mov ax, 0x30    ; GDT_TSS_SEL (6<<3)
    ltr ax
    ret
