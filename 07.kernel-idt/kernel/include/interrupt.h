#ifndef _INTERRUPT_H
#define _INTERRUPT_H

/*
 * interrupt.h
 * 32-bit x86 (Protected Mode) IDT / interrupt interface
 *
 * Strict, unambiguous, minimal public API & data layout for IDT entries.
 *
 * NOTE:
 *  - This header targets 32-bit protected mode IDT descriptors (8 bytes each).
 *  - All struct field names map directly to Intel descriptor bitfields.
 *  - The stack/register frame pushed by the assembly ISR stub must match
 *    the interrupt_frame_t layout below (order & presence).
 */

#include <types.h> /* provide uint8_t/uint16_t/uint32_t */

/* ---------------------------------------------------------------------------
 * IDT descriptor (32-bit interrupt gate) — exactly 8 bytes (Intel spec)
 *
 * Layout (low address -> high address):
 *  offset_low (bits 0..15)
 *  selector   (bits 16..31)
 *  reserved   (bits 32..39)    // must be zero
 *  type_attr  (bits 40..47)    // P, DPL, S/0, Type
 *  offset_high(bits 48..63)
 *
 * This struct MUST be packed and exactly match the 8-byte layout.
 * -------------------------------------------------------------------------*/
typedef struct idt_entry
{
    uint16_t offset_low;  /* handler address bits 0..15 */
    uint16_t selector;    /* code segment selector (usually kernel CS) */
    uint8_t reserved;     /* must be zero */
    uint8_t type_attr;    /* present, DPL, gate type (interrupt/trap) */
    uint16_t offset_high; /* handler address bits 16..31 */
} __attribute__((packed)) idt_entry_t;

/* IDT pointer used by lidt (6 bytes) */
typedef struct idt_ptr
{
    uint16_t limit; /* size of table - 1 */
    uint32_t base;  /* base address of IDT */
} __attribute__((packed)) idt_ptr_t;

/* ---------------------------------------------------------------------------
 * Interrupt frame — layout that the C handler will receive.
 *
 * This describes the stack contents as the assembly stub must push them
 * before calling the common C-level handler. **The assembly stub MUST
 * construct an instance of this frame in exactly this order.**
 *
 * Typical stub sequence (recommended canonical order):
 *   pushad           ; push eax,ecx,edx,ebx,esp_dummy,ebp,esi,edi (PUSHA order)
 *   push ds
 *   mov ds, KERNEL_DS ; if needed
 *   push <int_no>
 *   push <err_code or 0>
 *   push eip
 *   push cs
 *   push eflags
 *   push useresp (if coming from user) ; optional
 *   push ss (if coming from user)      ; optional
 *
 * To avoid conditional stub complexity, a common design is:
 *  - always push an err_code (0 if none)
 *  - always push useresp/ss placeholders (0) when not from user
 *
 * The following struct matches the common convention used by many tutorials.
 * IMPORTANT: keep this struct synchronized with your assembly stub.
 * -------------------------------------------------------------------------*/
typedef struct interrupt_frame
{
    /* pushed manually (optional); segment selector used by handler */
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    /* registers pushed by pusha (in this order: edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax) */
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy; /* original ESP pushed by pusha; not the user stack pointer */
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    /* pushed by stub: int number and error code (error code = 0 if none) */
    uint32_t int_no;
    uint32_t err_code;
    /* pushed by CPU on interrupt/exception */
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t useresp; /* only valid when privilege change occurred; otherwise 0 */
    uint32_t ss;      /* only valid when privilege change occurred; otherwise 0 */
} interrupt_frame_t;

/* ---------------------------------------------------------------------------
 * Public constants and helper macros
 * -------------------------------------------------------------------------*/
// IDT limit (256 entries)
#define IDT_ENTRIES 256

/* Build type_attr byte for IDT entries:
 *  P (bit7) | DPL (bits6-5) | 0 (bit4) | gate type (bits3-0)
 * Common types:
 *  0xE = 32-bit interrupt gate (0b1110)
 *  0xF = 32-bit trap gate      (0b1111)
 */
#define IDT_TYPE_INTERRUPT 0x0E
#define IDT_TYPE_TRAP 0x0F
#define IDT_PRESENT 0x80
#define IDT_DPL0 0x00
#define IDT_DPL3 0x60

/* Convenience macro to form type_attr */
#define IDT_MAKE_ATTR(present, dpl, type) (uint8_t)(((present) ? IDT_PRESENT : 0) | (dpl) | ((type) & 0x0F))

/* ---------------------------------------------------------------------------
 * Minimal public API (only necessary functions)
 *
 * - idt_init()        : initialize IDT entries and load IDT (lidt)
 * - register_interrupt_handler(n, h) : register C handler called by common ISR
 * - pic_init(), pic_send_eoi(irq)    : PIC utilities
 * - enable/disable interrupts        : inline wrappers for sti/cli
 *
 * Implementation notes:
 *  - Do NOT expose assembly ISR symbols (isr0..isr47) here; keep them internal.
 *  - The implementation must ensure the assembly stubs and interrupt_frame_t agree.
 * -------------------------------------------------------------------------*/

/* initialize IDT (set entries you need and load via lidt).
 * Should be called early in kernel init (after GDT/TSS). */
void idt_init(void);

/* Register a C-level handler for a vector (0..255). The handler is invoked with
 * a pointer to an interrupt_frame_t constructed by the assembly stub.
 * Passing NULL clears the handler.
 */
typedef void (*interrupt_handler_t)(interrupt_frame_t *frame);
void register_interrupt_handler(uint8_t vector, interrupt_handler_t handler);

/* PIC (8259) helpers:
 *  - pic_init(): remap master/slave to vectors (typically 0x20/0x28)
 *  - pic_send_eoi(irq): send End-Of-Interrupt for given IRQ number (0..15)
 */
void pic_init(uint8_t offset_vector_master, uint8_t offset_vector_slave);
void pic_send_eoi(uint8_t irq);

/* enable/disable CPU interrupts */
static inline void enable_interrupts(void) { __asm__ volatile("sti"); }
static inline void disable_interrupts(void) { __asm__ volatile("cli"); }

void isr_handler(interrupt_frame_t *frame);
void irq_handler(interrupt_frame_t *frame);

/* Query helper (optional) */
bool_t is_in_interrupt_context(void);

#endif /* _INTERRUPT_H */
