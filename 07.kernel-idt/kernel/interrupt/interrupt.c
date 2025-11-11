/*
 * interrupt.c
 * 32-bit x86 (Protected Mode) IDT / interrupt implementation
 */

#include "interrupt.h"
#include "types.h"
#include "gdt.h"
#include "io.h"
#include "ports.h"
#include "vga.h"

/* ---------------------------------------------------------------------------
 * Internal data structures and variables
 * -------------------------------------------------------------------------*/

// 中断处理程序数组，最多256个
static interrupt_handler_t interrupt_handlers[IDT_ENTRIES] = {0};

// IDT表和指针
static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

// 用于跟踪是否在中断上下文中
static volatile bool_t in_interrupt_context = FALSE;

// 汇编函数声明
// ISR（Interrupt Service Routine） 是CPU 异常或陷阱（exception/trap）的中断服务例程，这些异常是 CPU 硬件定义的，编号固定，不可更改。
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

// IRQ处理程序
// IRQ 是外部硬件中断，由可编程中断控制器（PIC）产生。例如键盘、定时器、硬盘、网卡等设备都有其默认的IRQ号(可重定义)；
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/* ---------------------------------------------------------------------------
 * IDT entry management functions
 * -------------------------------------------------------------------------*/

// 设置IDT条目
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].reserved = 0;
    idt[num].type_attr = flags;
}

/*
 * exception_names[] — CPU 异常名称表
 *
 * 每个异常对应中断向量号 0–31。
 * 这些由 CPU 硬件预定义，操作系统不得占用此范围。
 * 通常硬件中断（IRQ）从 0x20（32）号起开始使用。
 */

static const char *exception_names[32] = {
    /* 0 */ "Divide Error (#DE)",                     // 除零异常 (DIV/IDIV)
    /* 1 */ "Debug (#DB)",                            // 单步或调试陷阱
    /* 2 */ "Non-Maskable Interrupt (NMI)",           // 不可屏蔽中断信号
    /* 3 */ "Breakpoint (#BP)",                       // INT3 指令触发的断点
    /* 4 */ "Overflow (#OF)",                         // INTO 指令检测 OF 标志
    /* 5 */ "Bound Range Exceeded (#BR)",             // BOUND 指令越界
    /* 6 */ "Invalid Opcode (#UD)",                   // 无效或未定义指令
    /* 7 */ "Device Not Available (#NM)",             // FPU 被禁用或不存在
    /* 8 */ "Double Fault (#DF)",                     // 双重故障（错误中断嵌套错误）
    /* 9 */ "Coprocessor Segment Overrun (reserved)", // 旧协处理器错误（386 后弃用）
    /*10 */ "Invalid TSS (#TS)",                      // 加载无效 TSS
    /*11 */ "Segment Not Present (#NP)",              // 段不存在（加载段描述符失败）
    /*12 */ "Stack-Segment Fault (#SS)",              // 栈段越界或无效
    /*13 */ "General Protection Fault (#GP)",         // 通用保护异常（访问违规等）
    /*14 */ "Page Fault (#PF)",                       // 页错误（分页机制触发）
    /*15 */ "Reserved",                               // Intel 保留
    /*16 */ "x87 Floating-Point Exception (#MF)",     // FPU 浮点错误
    /*17 */ "Alignment Check (#AC)",                  // 非对齐内存访问（仅 CPL=3）
    /*18 */ "Machine Check (#MC)",                    // 机器检测错误（硬件故障）
    /*19 */ "SIMD Floating-Point Exception (#XF)",    // SSE/AVX 浮点异常
    /*20 */ "Virtualization Exception (#VE)",         // VMX 虚拟化异常
    /*21 */ "Control Protection Exception (#CP)",     // CET 控制流保护错误（新CPU）
    /*22 */ "Reserved",
    /*23 */ "Reserved",
    /*24 */ "Reserved",
    /*25 */ "Reserved",
    /*26 */ "Reserved",
    /*27 */ "Reserved",
    /*28 */ "Reserved",
    /*29 */ "Reserved",
    /*30 */ "Reserved",
    /*31 */ "Reserved"};

/* ---------------------------------------------------------------------------
 * Common interrupt handler
 * -------------------------------------------------------------------------*/

// 通用中断处理函数（由汇编调用）
void isr_handler(interrupt_frame_t *frame)
{
    // 设置中断上下文标志
    in_interrupt_context = TRUE;

    // 调用注册的处理程序（如果有）
    if (interrupt_handlers[frame->int_no] != 0)
    {
        interrupt_handler_t handler = interrupt_handlers[frame->int_no];
        handler(frame);
    }
    else
    {
        // 如果没有注册处理程序，打印异常信息
        if (frame->int_no < 32)
        {
            vga_printf("\nUnhandled Exception: %s (0x%x)\n",
                       exception_names[frame->int_no], frame->int_no);
            vga_printf("Error Code: 0x%x\n", frame->err_code);
            vga_printf("EIP: 0x%x, CS: 0x%x, EFLAGS: 0x%x\n",
                       frame->eip, frame->cs, frame->eflags);

            // 对于严重异常，停止系统
            if (frame->int_no == 8 || frame->int_no == 13 || frame->int_no == 14)
            {
                vga_printf("System halted due to critical exception.\n");
                __asm__ volatile("cli; hlt");
            }
        }
        else
        {
            vga_printf("\nUnhandled Interrupt: 0x%x\n", frame->int_no);
        }
    }

    // 清除中断上下文标志
    in_interrupt_context = FALSE;
}

// 通用IRQ处理函数（由汇编调用）
void irq_handler(interrupt_frame_t *frame)
{
    // 设置中断上下文标志
    in_interrupt_context = TRUE;

    // 发送EOI信号
    uint8_t irq = frame->int_no - 32; // IRQ号 = 中断向量 - 32
    pic_send_eoi(irq);

    // 调用注册的处理程序（如果有）
    if (interrupt_handlers[frame->int_no] != 0)
    {
        interrupt_handler_t handler = interrupt_handlers[frame->int_no];
        handler(frame);
    }
    else
    {
        vga_printf("\nUnhandled IRQ: 0x%x\n", frame->int_no);
    }

    // 清除中断上下文标志
    in_interrupt_context = FALSE;
}

/* ---------------------------------------------------------------------------
 * Public API implementation
 * -------------------------------------------------------------------------*/

#define SET_ISR(n) idt_set_gate((n), (uint32_t)isr##n, \
                                GDT_KERNEL_CODE_SEL, IDT_MAKE_ATTR(1, IDT_DPL0, IDT_TYPE_INTERRUPT))

#define SET_IRQ(n) idt_set_gate(32 + (n), (uint32_t)irq##n, \
                                GDT_KERNEL_CODE_SEL, IDT_MAKE_ATTR(1, IDT_DPL0, IDT_TYPE_INTERRUPT))
void idt_init(void)
{
    // 初始化IDT指针
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;
    idt_ptr.base = (uint32_t)&idt;

    // 清空IDT表
    for (int i = 0; i < IDT_ENTRIES; i++)
    {
        idt_set_gate(i, 0, 0, 0);
    }

    //==================================================
    // 注册 CPU 异常（Interrupt Service Routines, 0–31）
    //==================================================
    SET_ISR(0);  // #DE 除零错误
    SET_ISR(1);  // #DB 调试
    SET_ISR(2);  // NMI 非屏蔽中断
    SET_ISR(3);  // #BP 断点
    SET_ISR(4);  // #OF 溢出
    SET_ISR(5);  // #BR 越界
    SET_ISR(6);  // #UD 无效指令
    SET_ISR(7);  // #NM 无协处理器
    SET_ISR(8);  // #DF 双重错误
    SET_ISR(9);  // 协处理器段溢出（保留）
    SET_ISR(10); // #TS 无效 TSS
    SET_ISR(11); // #NP 段不存在
    SET_ISR(12); // #SS 栈段错误
    SET_ISR(13); // #GP 通用保护错误
    SET_ISR(14); // #PF 页错误
    SET_ISR(15); // 未使用（保留）
    SET_ISR(16); // #MF x87 浮点异常
    SET_ISR(17); // #AC 对齐检查
    SET_ISR(18); // #MC 机器检查
    SET_ISR(19); // #XM SIMD 浮点异常
    SET_ISR(20); // #VE 虚拟化异常
    SET_ISR(21);
    SET_ISR(22);
    SET_ISR(23);
    SET_ISR(24);
    SET_ISR(25);
    SET_ISR(26);
    SET_ISR(27);
    SET_ISR(28);
    SET_ISR(29);
    SET_ISR(30);
    SET_ISR(31);

    //==================================================
    // 注册可屏蔽中断（IRQ 0–15）
    //==================================================
    SET_IRQ(0);  // 系统定时器
    SET_IRQ(1);  // 键盘
    SET_IRQ(2);  // 级联（连接主从 PIC）
    SET_IRQ(3);  // 串口 COM2
    SET_IRQ(4);  // 串口 COM1
    SET_IRQ(5);  // LPT2 或其他设备
    SET_IRQ(6);  // 软盘控制器
    SET_IRQ(7);  // LPT1 / 打印机 / 并口
    SET_IRQ(8);  // 实时时钟
    SET_IRQ(9);  // 重定向 IRQ2（可用于网卡）
    SET_IRQ(10); // 保留 / 自定义设备
    SET_IRQ(11); // 保留 / 自定义设备
    SET_IRQ(12); // PS/2 鼠标
    SET_IRQ(13); // 协处理器 / FPU
    SET_IRQ(14); // 主 IDE 通道
    SET_IRQ(15); // 从 IDE 通道

    // 初始化PIC(Programmable Interrupt Controller)
    pic_init(PIC1_VECTOR_OFFSET, PIC2_VECTOR_OFFSET);

    // 加载IDT
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));

    vga_printf("[IDT] Initialized with %d entries.\n", IDT_ENTRIES);
}

void register_interrupt_handler(uint8_t vector, interrupt_handler_t handler)
{
    if (vector < IDT_ENTRIES)
    {
        interrupt_handlers[vector] = handler;
    }
    if (interrupt_handlers[vector] != NULL)
        vga_printf("[WARN] Overwriting handler for vector %d\n", vector);
}

void pic_init(uint8_t offset_vector_master, uint8_t offset_vector_slave)
{
    /* 保存当前中断屏蔽状态 */
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    /* 开始初始化序列 */
    outb(PIC1_CMD, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();
    outb(PIC2_CMD, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();

    /* ICW2: 设置中断向量偏移 */
    outb(PIC1_DATA, offset_vector_master);
    io_wait();
    outb(PIC2_DATA, offset_vector_slave);
    io_wait();

    /* ICW3: 设置主从级联关系 */
    outb(PIC1_DATA, PIC_MASTER_ICW3_IRQ2);
    io_wait();
    outb(PIC2_DATA, PIC_SLAVE_ICW3_ID);
    io_wait();

    /* ICW4: 设置工作模式 */
    outb(PIC1_DATA, PIC_ICW4_8086_MODE);
    io_wait();
    outb(PIC2_DATA, PIC_ICW4_8086_MODE);
    io_wait();

    /* 恢复原有屏蔽位 */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);

    vga_printf("[PIC] Initialized: master offset=0x%x, slave offset=0x%x\n",
               offset_vector_master, offset_vector_slave);
}

// 处理完中断后需要告诉 PIC，本次中断已处理，允许 PIC 发下一个中断。
void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
    {
        // 如果是从PIC的IRQ，需要向从PIC发送EOI
        outb(PIC2_CMD, PIC_EOI);
    }
    // 总是向主PIC发送EOI
    outb(PIC1_CMD, PIC_EOI);
}

bool_t is_in_interrupt_context(void)
{
    return in_interrupt_context;
}