#ifndef PORTS_H
#define PORTS_H

/* ----------------------------------------------------------
 * 中断向量定义
 * ---------------------------------------------------------- */
typedef enum
{
    /* 异常 (0-31) */
    INT_DIVIDE_ERROR = 0,         /* 除零错误 */
    INT_DEBUG = 1,                /* 调试异常 */
    INT_NMI = 2,                  /* 非屏蔽中断 */
    INT_BREAKPOINT = 3,           /* 断点异常 */
    INT_OVERFLOW = 4,             /* 溢出异常 */
    INT_BOUND_RANGE = 5,          /* 边界检查异常 */
    INT_INVALID_OPCODE = 6,       /* 无效操作码 */
    INT_DEVICE_NOT_AVAILABLE = 7, /* 设备不可用(协处理器不存在) */
    INT_DOUBLE_FAULT = 8,         /* 双重故障 */
    INT_COPROCESSOR_SEGMENT = 9,  /* 协处理器段溢出(保留) */
    INT_INVALID_TSS = 10,         /* 无效TSS */
    INT_SEGMENT_NOT_PRESENT = 11, /* 段不存在 */
    INT_STACK_SEGMENT = 12,       /* 栈段故障 */
    INT_GENERAL_PROTECTION = 13,  /* 一般保护性异常 */
    INT_PAGE_FAULT = 14,          /* 页故障 */
    INT_RESERVED_15 = 15,         /* 保留 */
    INT_X87_FPU_ERROR = 16,       /* x87 FPU错误 */
    INT_ALIGNMENT_CHECK = 17,     /* 对齐检查 */
    INT_MACHINE_CHECK = 18,       /* 机器检查 */
    INT_SIMD_FP = 19,             /* SIMD浮点异常 */
    INT_VIRTUALIZATION = 20,      /* 虚拟化异常 */
    INT_RESERVED_21 = 21,         /* 保留 */
    INT_RESERVED_22 = 22,         /* 保留 */
    INT_RESERVED_23 = 23,         /* 保留 */
    INT_RESERVED_24 = 24,         /* 保留 */
    INT_RESERVED_25 = 25,         /* 保留 */
    INT_RESERVED_26 = 26,         /* 保留 */
    INT_RESERVED_27 = 27,         /* 保留 */
    INT_RESERVED_28 = 28,         /* 保留 */
    INT_RESERVED_29 = 29,         /* 保留 */
    INT_RESERVED_30 = 30,         /* 保留 */
    INT_RESERVED_31 = 31,         /* 保留 */

    /* IRQ 中断 (32-47) */
    INT_IRQ0 = 32,  /* 系统定时器(PIT) */
    INT_IRQ1 = 33,  /* 键盘 */
    INT_IRQ2 = 34,  /* 级联(用于从PIC) */
    INT_IRQ3 = 35,  /* COM2/COM4 */
    INT_IRQ4 = 36,  /* COM1/COM3 */
    INT_IRQ5 = 37,  /* LPT2 */
    INT_IRQ6 = 38,  /* 软盘驱动器 */
    INT_IRQ7 = 39,  /* LPT1 */
    INT_IRQ8 = 40,  /* 实时时钟(RTC) */
    INT_IRQ9 = 41,  /* 自由IRQ(重定向到IRQ2) */
    INT_IRQ10 = 42, /* 自由IRQ */
    INT_IRQ11 = 43, /* 自由IRQ */
    INT_IRQ12 = 44, /* PS/2鼠标 */
    INT_IRQ13 = 45, /* 数学协处理器 */
    INT_IRQ14 = 46, /* 主IDE */
    INT_IRQ15 = 47, /* 从IDE */

    /* 软件中断 (48+) */
    INT_SYSCALL = 48,  /* 系统调用 */
    INT_DEBUGGER = 49, /* 调试器 */
    /* ... 其他软件中断 */
} interrupt_vector_t;

/* ----------------------------------------------------------
 * 8259A Programmable Interrupt Controller
 * ---------------------------------------------------------- */
#define PIC1_CMD 0x20      /* 主PIC命令端口 */
#define PIC1_DATA 0x21     /* 主PIC数据端口 */
#define PIC2_CMD 0xA0      /* 从PIC命令端口 */
#define PIC2_DATA 0xA1     /* 从PIC数据端口 */
#define PIC_EOI 0x20       /* 非特定EOI命令 */
#define PIC_INIT 0x11      /* 初始化命令 */
#define PIC_ICW4_8086 0x01 /* ICW4: 8086模式 */

/* ICW1: 初始化控制字 1 */
#define PIC_ICW1_ICW4 0x01      /* 需要 ICW4 */
#define PIC_ICW1_SINGLE 0x02    /* 单个 PIC（不级联） */
#define PIC_ICW1_INTERVAL4 0x04 /* 调整中断向量间隔 */
#define PIC_ICW1_LEVEL 0x08     /* 电平触发（默认边沿） */
#define PIC_ICW1_INIT 0x10      /* 启动初始化 */

/* ICW3: 主从连接信息 */
#define PIC_MASTER_ICW3_IRQ2 0x04 /* 主PIC连接从PIC在IRQ2线上 */
#define PIC_SLAVE_ICW3_ID 0x02    /* 从PIC的ID号（连接在IRQ2） */

/* ICW4: 初始化控制字 4 */
#define PIC_ICW4_8086_MODE 0x01  /* 8086/88 模式（非MCS-80/85） */
#define PIC_ICW4_AUTO_EOI 0x02   /* 自动EOI */
#define PIC_ICW4_BUF_SLAVE 0x08  /* 缓冲模式（从PIC） */
#define PIC_ICW4_BUF_MASTER 0x0C /* 缓冲模式（主PIC） */
#define PIC_ICW4_SFNM 0x10       /* 特殊完全嵌套模式 */

/* 自定义的中断向量偏移值 */
#define PIC1_VECTOR_OFFSET 0x20 /* IRQ0–7 → IDT 32–39 */
#define PIC2_VECTOR_OFFSET 0x28 /* IRQ8–15 → IDT 40–47 */

/* ----------------------------------------------------------
 * 8254 PIT (Programmable Interval Timer)
 * ---------------------------------------------------------- */

#define PIT_CH0 0x40 /* 通道0 (系统定时器) */
#define PIT_CH1 0x41 /* 通道1 (内存刷新) */
#define PIT_CH2 0x42 /* 通道2 (PC扬声器) */
#define PIT_CMD 0x43 /* 命令端口 */

/* PIT命令位定义 */
#define PIT_CHANNEL0 0x00     /* 选择通道0 */
#define PIT_CHANNEL1 0x40     /* 选择通道1 */
#define PIT_CHANNEL2 0x80     /* 选择通道2 */
#define PIT_ACCESS_LATCH 0x00 /* 锁存计数值 */
#define PIT_ACCESS_LOW 0x10   /* 只访问低字节 */
#define PIT_ACCESS_HIGH 0x20  /* 只访问高字节 */
#define PIT_ACCESS_BOTH 0x30  /* 先低后高字节 */
#define PIT_MODE0 0x00        /* 模式0: 中断计数 */
#define PIT_MODE1 0x02        /* 模式1: 硬件可重触发单稳态 */
#define PIT_MODE2 0x04        /* 模式2: 速率发生器 */
#define PIT_MODE3 0x06        /* 模式3: 方波发生器 */
#define PIT_MODE4 0x08        /* 模式4: 软件触发选通 */
#define PIT_MODE5 0x0A        /* 模式5: 硬件触发选通 */
#define PIT_BINARY 0x00       /* 二进制计数 */
#define PIT_BCD 0x01          /* BCD计数 */

/* 常用频率设置 */
#define PIT_FREQ_BASE 1193180 /* PIT基准频率(Hz) */
#define PIT_FREQ_100HZ 100    /* 100Hz中断频率 */
#define PIT_FREQ_1000HZ 1000  /* 1000Hz中断频率 */

/* ----------------------------------------------------------
 * 8042 Keyboard / PS/2 Mouse & Controller
 * ---------------------------------------------------------- */
#define KBD_DATA 0x60   /* 键盘数据端口 */
#define KBD_STATUS 0x64 /* 键盘状态端口 (读) */
#define KBD_CMD 0x64    /* 键盘命令端口 (写) */

/* 键盘状态位定义 */
#define KBD_STATUS_OUT_FULL 0x01 /* 输出缓冲区满 */
#define KBD_STATUS_IN_FULL 0x02  /* 输入缓冲区满 */
#define KBD_STATUS_SYSTEM 0x04   /* 系统 */
#define KBD_STATUS_CMD_DATA 0x08 /* 命令/数据 */
#define KBD_STATUS_TIMEOUT 0x40  /* 超时错误 */
#define KBD_STATUS_PARITY 0x80   /* 奇偶校验错误 */

/* 键盘命令 */
#define KBD_CMD_SET_LEDS 0xED /* 设置LED状态 */
#define KBD_CMD_ECHO 0xEE     /* 回显命令 */
#define KBD_CMD_GET_ID 0xF2   /* 获取设备ID */
#define KBD_CMD_SET_RATE 0xF3 /* 设置重复速率 */
#define KBD_CMD_ENABLE 0xF4   /* 启用扫描 */
#define KBD_CMD_DISABLE 0xF5  /* 禁用扫描 */
#define KBD_CMD_RESET 0xFF    /* 重置 */

/* 键盘扫描码集 */
typedef enum
{
    KBD_SCANCODE_SET_1 = 1,
    KBD_SCANCODE_SET_2 = 2,
    KBD_SCANCODE_SET_3 = 3
} kbd_scancode_set_t;

/* ----------------------------------------------------------
 * CMOS / RTC (MC146818)
 * ---------------------------------------------------------- */
#define CMOS_ADDR 0x70 /* CMOS地址端口 */
#define CMOS_DATA 0x71 /* CMOS数据端口 */

/* CMOS寄存器地址 */
#define CMOS_SECONDS 0x00       /* 秒 */
#define CMOS_SECONDS_ALARM 0x01 /* 秒闹钟 */
#define CMOS_MINUTES 0x02       /* 分 */
#define CMOS_MINUTES_ALARM 0x03 /* 分闹钟 */
#define CMOS_HOURS 0x04         /* 时 */
#define CMOS_HOURS_ALARM 0x05   /* 时闹钟 */
#define CMOS_WEEKDAY 0x06       /* 星期 */
#define CMOS_DAY_OF_MONTH 0x07  /* 日 */
#define CMOS_MONTH 0x08         /* 月 */
#define CMOS_YEAR 0x09          /* 年 */
#define CMOS_STATUS_A 0x0A      /* 状态寄存器A */
#define CMOS_STATUS_B 0x0B      /* 状态寄存器B */
#define CMOS_STATUS_C 0x0C      /* 状态寄存器C */
#define CMOS_STATUS_D 0x0D      /* 状态寄存器D */
#define CMOS_DIAGNOSTIC 0x0E    /* 诊断状态 */
#define CMOS_SHUTDOWN 0x0F      /* 关机状态 */

/* CMOS状态寄存器B位定义 */
#define CMOS_B_DST 0x01    /* 夏令时 */
#define CMOS_B_24HR 0x02   /* 24小时格式 */
#define CMOS_B_BINARY 0x04 /* 二进制模式 */
#define CMOS_B_SQWE 0x08   /* 方波使能 */
#define CMOS_B_UIE 0x10    /* 更新结束中断使能 */
#define CMOS_B_AIE 0x20    /* 闹钟中断使能 */
#define CMOS_B_PIE 0x40    /* 周期中断使能 */
#define CMOS_B_SET 0x80    /* 设置时间 */

/* ----------------------------------------------------------
 * Serial UART 16550 (COM1~COM4)
 * ---------------------------------------------------------- */
#define COM1_BASE 0x3F8 /* COM1基址 */
#define COM2_BASE 0x2F8 /* COM2基址 */
#define COM3_BASE 0x3E8 /* COM3基址 */
#define COM4_BASE 0x2E8 /* COM4基址 */

/* UART寄存器偏移 */
#define UART_RX 0  /* 接收缓冲器 (DLAB=0) */
#define UART_TX 0  /* 发送保持寄存器 (DLAB=0) */
#define UART_DLL 0 /* 波特率除数锁存器低字节 (DLAB=1) */
#define UART_DLM 1 /* 波特率除数锁存器高字节 (DLAB=1) */
#define UART_IER 1 /* 中断允许寄存器 (DLAB=0) */
#define UART_IIR 2 /* 中断识别寄存器 */
#define UART_FCR 2 /* FIFO控制寄存器 */
#define UART_LCR 3 /* 线控制寄存器 */
#define UART_MCR 4 /* MODEM控制寄存器 */
#define UART_LSR 5 /* 线状态寄存器 */
#define UART_MSR 6 /* MODEM状态寄存器 */
#define UART_SCR 7 /* 暂存寄存器 */

/* 宏：根据基址生成端口 */
#define UART_PORT(base, reg) ((base) + (reg))

/* UART线控制寄存器位定义 */
#define UART_LCR_DLAB 0x80   /* 除数锁存访问位 */
#define UART_LCR_BREAK 0x40  /* 设置中断 */
#define UART_LCR_PARITY 0x38 /* 奇偶校验位掩码 */
#define UART_LCR_STOP 0x04   /* 停止位 */
#define UART_LCR_DLEN 0x03   /* 数据位长度掩码 */

/* UART线状态寄存器位定义 */
#define UART_LSR_RDR 0x01  /* 接收数据就绪 */
#define UART_LSR_OE 0x02   /* 溢出错误 */
#define UART_LSR_PE 0x04   /* 奇偶校验错误 */
#define UART_LSR_FE 0x08   /* 帧错误 */
#define UART_LSR_BI 0x10   /* 中断指示 */
#define UART_LSR_THRE 0x20 /* 发送保持寄存器空 */
#define UART_LSR_TEMT 0x40 /* 发送器空 */
#define UART_LSR_ERR 0x80  /* 错误指示 */

/* UART FIFO控制寄存器位定义 */
#define UART_FCR_ENABLE 0x01     /* 使能FIFO */
#define UART_FCR_CLEAR_RX 0x02   /* 清除接收FIFO */
#define UART_FCR_CLEAR_TX 0x04   /* 清除发送FIFO */
#define UART_FCR_DMA_MODE 0x08   /* DMA模式 */
#define UART_FCR_TRIGGER_1 0x00  /* 触发级别1 */
#define UART_FCR_TRIGGER_4 0x40  /* 触发级别4 */
#define UART_FCR_TRIGGER_8 0x80  /* 触发级别8 */
#define UART_FCR_TRIGGER_14 0xC0 /* 触发级别14 */

/* 常用波特率设置 */
#define UART_BAUD_1200 96
#define UART_BAUD_2400 48
#define UART_BAUD_4800 24
#define UART_BAUD_9600 12
#define UART_BAUD_19200 6
#define UART_BAUD_38400 3
#define UART_BAUD_57600 2
#define UART_BAUD_115200 1

/* ----------------------------------------------------------
 * VGA / CGA / MDA 显示适配器寄存器
 * ---------------------------------------------------------- */
#define VGA_CRTC_ADDR 0x3D4   /* CRT控制器索引寄存器 */
#define VGA_CRTC_DATA 0x3D5   /* CRT控制器数据寄存器 */
#define VGA_SEQ_ADDR 0x3C4    /* 序列器索引寄存器 */
#define VGA_SEQ_DATA 0x3C5    /* 序列器数据寄存器 */
#define VGA_GFX_ADDR 0x3CE    /* 图形控制器索引寄存器 */
#define VGA_GFX_DATA 0x3CF    /* 图形控制器数据寄存器 */
#define VGA_ATTR_ADDR 0x3C0   /* 属性控制器索引寄存器 (复用) */
#define VGA_ATTR_DATA 0x3C1   /* 属性控制器数据寄存器 (读) */
#define VGA_ATTR_WRITE 0x3C0  /* 属性控制器数据寄存器 (写) */
#define VGA_INSTAT_READ 0x3DA /* 输入状态寄存器 (读) */
#define VGA_MISC_READ 0x3CC   /* 杂项输出寄存器 (读) */
#define VGA_MISC_WRITE 0x3C2  /* 杂项输出寄存器 (写) */
#define VGA_FEATURE_R 0x3CA   /* 特征控制寄存器 (读) */
#define VGA_FEATURE_W 0x3DA   /* 特征控制寄存器 (写) */

/* CRT控制器内部寄存器索引 */
#define VGA_CRTC_H_TOTAL 0x00         /* 水平总计 */
#define VGA_CRTC_H_DISP_END 0x01      /* 水平显示结束 */
#define VGA_CRTC_H_BLANK_START 0x02   /* 水平消隐开始 */
#define VGA_CRTC_H_BLANK_END 0x03     /* 水平消隐结束 */
#define VGA_CRTC_H_RETRACE_START 0x04 /* 水平回扫开始 */
#define VGA_CRTC_H_RETRACE_END 0x05   /* 水平回扫结束 */
#define VGA_CRTC_V_TOTAL 0x06         /* 垂直总计 */
#define VGA_CRTC_OVERFLOW 0x07        /* 溢出寄存器 */
#define VGA_CRTC_PRESET_ROW 0x08      /* 预设行扫描 */
#define VGA_CRTC_MAX_SCAN 0x09        /* 最大扫描线 */
#define VGA_CRTC_CURSOR_START 0x0A    /* 光标开始 */
#define VGA_CRTC_CURSOR_END 0x0B      /* 光标结束 */
#define VGA_CRTC_START_ADDR_HI 0x0C   /* 开始地址高字节 */
#define VGA_CRTC_START_ADDR_LO 0x0D   /* 开始地址低字节 */
#define VGA_CRTC_CURSOR_LOC_HI 0x0E   /* 光标位置高字节 */
#define VGA_CRTC_CURSOR_LOC_LO 0x0F   /* 光标位置低字节 */
#define VGA_CRTC_V_RETRACE_START 0x10 /* 垂直回扫开始 */
#define VGA_CRTC_V_RETRACE_END 0x11   /* 垂直回扫结束 */
#define VGA_CRTC_V_DISP_END 0x12      /* 垂直显示结束 */
#define VGA_CRTC_OFFSET 0x13          /* 偏移 */
#define VGA_CRTC_UNDERLINE 0x14       /* 下划线位置 */
#define VGA_CRTC_V_BLANK_START 0x15   /* 垂直消隐开始 */
#define VGA_CRTC_V_BLANK_END 0x16     /* 垂直消隐结束 */
#define VGA_CRTC_MODE 0x17            /* 模式控制 */
#define VGA_CRTC_LINE_COMPARE 0x18    /* 行比较 */

/* 老式单色MDA端口 */
#define MDA_CRTC_ADDR 0x3B4 /* MDA CRT控制器索引寄存器 */
#define MDA_CRTC_DATA 0x3B5 /* MDA CRT控制器数据寄存器 */

/* VGA颜色定义 */
typedef enum
{
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15
} vga_color_t;

/* ----------------------------------------------------------
 * ATA/ATAPI (Primary & Secondary)
 * ---------------------------------------------------------- */
/* 主ATA通道 */
#define ATA_PRI_DATA 0x1F0     /* 数据寄存器 */
#define ATA_PRI_ERROR 0x1F1    /* 错误寄存器 (读) */
#define ATA_PRI_FEATURES 0x1F1 /* 特性寄存器 (写) */
#define ATA_PRI_SECCOUNT 0x1F2 /* 扇区计数 */
#define ATA_PRI_LBA0 0x1F3     /* LBA低字节 */
#define ATA_PRI_LBA1 0x1F4     /* LBA中字节 */
#define ATA_PRI_LBA2 0x1F5     /* LBA高字节 */
#define ATA_PRI_HDDEVSEL 0x1F6 /* 驱动器/磁头选择 */
#define ATA_PRI_COMMAND 0x1F7  /* 命令寄存器 (写) */
#define ATA_PRI_STATUS 0x1F7   /* 状态寄存器 (读) */
#define ATA_PRI_ALTSTAT 0x3F6  /* 替代状态寄存器 (读) */
#define ATA_PRI_CONTROL 0x3F6  /* 设备控制寄存器 (写) */

/* 从ATA通道 */
#define ATA_SEC_DATA 0x170     /* 数据寄存器 */
#define ATA_SEC_ERROR 0x171    /* 错误寄存器 (读) */
#define ATA_SEC_FEATURES 0x171 /* 特性寄存器 (写) */
#define ATA_SEC_SECCOUNT 0x172 /* 扇区计数 */
#define ATA_SEC_LBA0 0x173     /* LBA低字节 */
#define ATA_SEC_LBA1 0x174     /* LBA中字节 */
#define ATA_SEC_LBA2 0x175     /* LBA高字节 */
#define ATA_SEC_HDDEVSEL 0x176 /* 驱动器/磁头选择 */
#define ATA_SEC_COMMAND 0x177  /* 命令寄存器 (写) */
#define ATA_SEC_STATUS 0x177   /* 状态寄存器 (读) */
#define ATA_SEC_ALTSTAT 0x376  /* 替代状态寄存器 (读) */
#define ATA_SEC_CONTROL 0x376  /* 设备控制寄存器 (写) */

/* ATA状态位定义 */
#define ATA_STATUS_BSY 0x80  /* 忙 */
#define ATA_STATUS_DRDY 0x40 /* 设备就绪 */
#define ATA_STATUS_DF 0x20   /* 设备故障 */
#define ATA_STATUS_DSC 0x10  /* 寻道完成 */
#define ATA_STATUS_DRQ 0x08  /* 数据请求 */
#define ATA_STATUS_CORR 0x04 /* 校正 */
#define ATA_STATUS_IDX 0x02  /* 索引 */
#define ATA_STATUS_ERR 0x01  /* 错误 */

/* ATA命令 */
#define ATA_CMD_READ_PIO 0x20  /* 读扇区(PIO) */
#define ATA_CMD_WRITE_PIO 0x30 /* 写扇区(PIO) */
#define ATA_CMD_READ_DMA 0xC8  /* 读扇区(DMA) */
#define ATA_CMD_WRITE_DMA 0xCA /* 写扇区(DMA) */
#define ATA_CMD_IDENTIFY 0xEC  /* 识别设备 */

/* ATA设备类型 */
typedef enum
{
    ATA_DEV_ATA = 0,  /* ATA设备 */
    ATA_DEV_ATAPI = 1 /* ATAPI设备 */
} ata_device_type_t;

/* ----------------------------------------------------------
 * 软盘控制器 (FDC 8272)
 * ---------------------------------------------------------- */
#define FDC_DIGITAL_OUTPUT 0x3F2 /* 数字输出寄存器 */
#define FDC_MAIN_STATUS 0x3F4    /* 主状态寄存器 */
#define FDC_DATA 0x3F5           /* 数据寄存器 */
#define FDC_DIGITAL_INPUT 0x3F7  /* 数字输入寄存器 (读) */
#define FDC_CONFIG_CONTROL 0x3F7 /* 配置控制寄存器 (写) */

/* ----------------------------------------------------------
 * 游戏口 / Joystick
 * ---------------------------------------------------------- */
#define GAMEPORT_DATA 0x201 /* 游戏口数据寄存器 */

/* ----------------------------------------------------------
 * 扬声器 / PC 蜂鸣器
 * ---------------------------------------------------------- */
#define SPEAKER_DATA 0x61      /* 扬声器控制寄存器 */
#define SPEAKER_ENABLE 0x01    /* 位0: 启用扬声器 */
#define SPEAKER_USE_TIMER 0x02 /* 位1: 使用定时器2 */

/* ----------------------------------------------------------
 * DMA 控制器 8237
 * ---------------------------------------------------------- */
/* DMA控制器1 (通道0-3) */
#define DMA1_CH0_ADDR 0x00     /* 通道0地址寄存器 */
#define DMA1_CH0_COUNT 0x01    /* 通道0计数寄存器 */
#define DMA1_CH1_ADDR 0x02     /* 通道1地址寄存器 */
#define DMA1_CH1_COUNT 0x03    /* 通道1计数寄存器 */
#define DMA1_CH2_ADDR 0x04     /* 通道2地址寄存器 */
#define DMA1_CH2_COUNT 0x05    /* 通道2计数寄存器 */
#define DMA1_CH3_ADDR 0x06     /* 通道3地址寄存器 */
#define DMA1_CH3_COUNT 0x07    /* 通道3计数寄存器 */
#define DMA1_STATUS 0x08       /* 状态寄存器 */
#define DMA1_COMMAND 0x08      /* 命令寄存器 */
#define DMA1_REQUEST 0x09      /* 请求寄存器 */
#define DMA1_MASK_SINGLE 0x0A  /* 单通道屏蔽寄存器 */
#define DMA1_MODE 0x0B         /* 模式寄存器 */
#define DMA1_CLEAR_FF 0x0C     /* 清除先/后触发器 */
#define DMA1_MASTER_CLEAR 0x0D /* 主清除 */
#define DMA1_CLR_MASK 0x0E     /* 清除屏蔽寄存器 */
#define DMA1_MASK_ALL 0x0F     /* 所有通道屏蔽寄存器 */

/* DMA控制器2 (通道4-7) */
#define DMA2_CH4_ADDR 0xC0     /* 通道4地址寄存器 */
#define DMA2_CH4_COUNT 0xC2    /* 通道4计数寄存器 */
#define DMA2_CH5_ADDR 0xC4     /* 通道5地址寄存器 */
#define DMA2_CH5_COUNT 0xC6    /* 通道5计数寄存器 */
#define DMA2_CH6_ADDR 0xC8     /* 通道6地址寄存器 */
#define DMA2_CH6_COUNT 0xCA    /* 通道6计数寄存器 */
#define DMA2_CH7_ADDR 0xCC     /* 通道7地址寄存器 */
#define DMA2_CH7_COUNT 0xCE    /* 通道7计数寄存器 */
#define DMA2_STATUS 0xD0       /* 状态寄存器 */
#define DMA2_COMMAND 0xD0      /* 命令寄存器 */
#define DMA2_REQUEST 0xD2      /* 请求寄存器 */
#define DMA2_MASK_SINGLE 0xD4  /* 单通道屏蔽寄存器 */
#define DMA2_MODE 0xD6         /* 模式寄存器 */
#define DMA2_CLEAR_FF 0xD8     /* 清除先/后触发器 */
#define DMA2_MASTER_CLEAR 0xDA /* 主清除 */
#define DMA2_CLR_MASK 0xDC     /* 清除屏蔽寄存器 */
#define DMA2_MASK_ALL 0xDE     /* 所有通道屏蔽寄存器 */

/* DMA页寄存器 */
#define DMA_PAGE_0 0x87 /* 通道0页寄存器 */
#define DMA_PAGE_1 0x83 /* 通道1页寄存器 */
#define DMA_PAGE_2 0x81 /* 通道2页寄存器 */
#define DMA_PAGE_3 0x82 /* 通道3页寄存器 */
#define DMA_PAGE_5 0x8B /* 通道5页寄存器 */
#define DMA_PAGE_6 0x89 /* 通道6页寄存器 */
#define DMA_PAGE_7 0x8A /* 通道7页寄存器 */

/* DMA模式位定义 */
#define DMA_MODE_DEMAND 0x00   /* 按需传输 */
#define DMA_MODE_SINGLE 0x40   /* 单次传输 */
#define DMA_MODE_BLOCK 0x80    /* 块传输 */
#define DMA_MODE_CASCADE 0xC0  /* 级联模式 */
#define DMA_MODE_ADDR_INC 0x00 /* 地址递增 */
#define DMA_MODE_ADDR_DEC 0x20 /* 地址递减 */
#define DMA_MODE_ADDR_FIX 0x40 /* 地址固定 */
#define DMA_MODE_VERIFY 0x00   /* 校验传输 */
#define DMA_MODE_WRITE 0x04    /* 写传输 */
#define DMA_MODE_READ 0x08     /* 读传输 */

/* ----------------------------------------------------------
 * 可编程外设接口 (PPI) 8255A – 仅在 PC/XT 有用
 * ---------------------------------------------------------- */
#define PPI_PORT_A 0x60  /* 端口A */
#define PPI_PORT_B 0x61  /* 端口B */
#define PPI_PORT_C 0x62  /* 端口C */
#define PPI_CONTROL 0x63 /* 控制寄存器 */

/* ----------------------------------------------------------
 * 电源管理 / 即插即用 / 其他杂项
 * ---------------------------------------------------------- */
#define PORT_A20_MASK 0x92 /* Fast A20门控 */
#define IO_DELAY_PORT 0x80 /* 传统空操作端口 */

#endif /* PORTS_H */