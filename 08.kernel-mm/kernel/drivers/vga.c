#include <stdarg.h>
#include "vga.h"
#include "io.h"
#include "ports.h"
#include "string.h"

static uint16_t *const VGA_BUF = (uint16_t *)VGA_MEMORY;
static uint8_t cursor_row = 0;
static uint8_t cursor_col = 0;
static uint8_t current_color = VGA_WHITE_ON_BLACK;

/* 同步硬件光标位置 */
static void vga_sync_cursor(void)
{
    uint16_t cursorLocation = cursor_row * 80 + cursor_col;
    outb(VGA_CRTC_ADDR, VGA_CRTC_CURSOR_LOC_HI);
    outb(VGA_CRTC_DATA, cursorLocation >> 8);
    outb(VGA_CRTC_ADDR, VGA_CRTC_CURSOR_LOC_LO);
    outb(VGA_CRTC_DATA, cursorLocation);
}

/* 移动光标到指定位置 */
void vga_move_cursor(uint8_t row, uint8_t col)
{
    if (row < VGA_HEIGHT && col < VGA_WIDTH)
    {
        cursor_row = row;
        cursor_col = col;
        vga_sync_cursor();
    }
}

/* 清屏 */
void vga_clear(void)
{
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    {
        VGA_BUF[i] = (current_color << 8) | ' ';
    }
    cursor_row = 0;
    cursor_col = 0;
    vga_sync_cursor();
}

/* 输出单个字符（自动换行和滚屏） */
void vga_putc(char c)
{
    if (c == '\n')
    {
        cursor_row++;
        cursor_col = 0;
    }
    else if (c == '\r')
    {
        cursor_col = 0;
    }
    else if (c == '\t')
    {
        /* 制表符：对齐到4的倍数 */
        cursor_col = (cursor_col + 4) & ~3;
        if (cursor_col >= VGA_WIDTH)
        {
            cursor_col = 0;
            cursor_row++;
        }
    }
    else if (c == '\b')
    {
        /* 退格：删除前一个字符 */
        if (cursor_col > 0)
        {
            cursor_col--;
            VGA_BUF[cursor_row * VGA_WIDTH + cursor_col] = (current_color << 8) | ' ';
        }
    }
    else
    {
        VGA_BUF[cursor_row * VGA_WIDTH + cursor_col] = (current_color << 8) | c;
        cursor_col++;
        if (cursor_col >= VGA_WIDTH)
        {
            cursor_col = 0;
            cursor_row++;
        }
    }

    /* 滚屏处理 */
    if (cursor_row >= VGA_HEIGHT)
    {
        /* 将所有行向上移动一行 */
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++)
        {
            VGA_BUF[i] = VGA_BUF[i + VGA_WIDTH];
        }
        /* 清空最后一行 */
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++)
        {
            VGA_BUF[i] = (current_color << 8) | ' ';
        }
        cursor_row = VGA_HEIGHT - 1;
    }

    vga_sync_cursor();
}

/* 输出字符串 */
void vga_write(const char *s)
{
    if (!s)
        return;

    while (*s)
    {
        vga_putc(*s++);
    }
}

/* 设置当前颜色 */
void vga_set_color(uint8_t color)
{
    current_color = color;
}

/* 将64位无符号整数转换为字符串（避免64位除法） */
static void vga_ulltoa(uint64_t value, char *buf, int base, size_t buf_size)
{
    if (buf_size == 0)
        return;

    char *p = buf;
    char *end = buf + buf_size - 1;

    if (base == 16)
    {
        /* 十六进制：使用移位和掩码，不需要除法 */
        if (value == 0)
        {
            if (p < end)
                *p++ = '0';
        }
        else
        {
            do
            {
                if (p >= end)
                    break;
                int digit = value & 0xF;
                *p++ = (digit < 10) ? '0' + digit : 'a' + (digit - 10);
                value >>= 4;
            } while (value != 0);
        }

        *p = '\0';

        /* 反转字符串 */
        for (char *a = buf, *b = p - 1; a < b; ++a, --b)
        {
            char t = *a;
            *a = *b;
            *b = t;
        }
    }
    else if (base == 10)
    {
        /* 十进制：使用减法代替除法（避免64位除法） */
        if (value == 0)
        {
            if (p < end)
                *p++ = '0';
            *p = '\0';
            return;
        }

        /* 10的幂表，从大到小 */
        static const uint64_t powers[] = {
            10000000000000000000ULL, /* 10^19 */
            1000000000000000000ULL,  /* 10^18 */
            100000000000000000ULL,   /* 10^17 */
            10000000000000000ULL,    /* 10^16 */
            1000000000000000ULL,     /* 10^15 */
            100000000000000ULL,      /* 10^14 */
            10000000000000ULL,       /* 10^13 */
            1000000000000ULL,        /* 10^12 */
            100000000000ULL,         /* 10^11 */
            10000000000ULL,          /* 10^10 */
            1000000000ULL,           /* 10^9 */
            100000000ULL,            /* 10^8 */
            10000000ULL,             /* 10^7 */
            1000000ULL,              /* 10^6 */
            100000ULL,               /* 10^5 */
            10000ULL,                /* 10^4 */
            1000ULL,                 /* 10^3 */
            100ULL,                  /* 10^2 */
            10ULL,                   /* 10^1 */
            1ULL                     /* 10^0 */
        };

        int leading_zero = 1; /* 跳过前导0标志 */

        for (size_t i = 0; i < sizeof(powers) / sizeof(powers[0]); i++)
        {
            if (p >= end)
                break;

            uint64_t power = powers[i];
            int digit = 0;

            /* 通过减法计算当前位的数字 */
            while (value >= power)
            {
                value -= power;
                digit++;
            }

            /* 跳过前导0 */
            if (leading_zero && digit == 0)
                continue;

            leading_zero = 0;
            *p++ = '0' + digit;
        }

        *p = '\0';
    }
    else
    {
        /* 不支持的进制 */
        if (p < end)
            *p++ = '?';
        *p = '\0';
    }
}

/* 将32位无符号整数转换为字符串 */
static void vga_utoa(uint32_t value, char *buf, int base, size_t buf_size)
{
    if (buf_size == 0)
        return;

    char *p = buf;
    char *end = buf + buf_size - 1;

    if (value == 0)
    {
        if (p < end)
            *p++ = '0';
        *p = '\0';
        return;
    }

    if (base == 16)
    {
        /* 十六进制：使用移位 */
        do
        {
            if (p >= end)
                break;
            int digit = value & 0xF;
            *p++ = (digit < 10) ? '0' + digit : 'a' + (digit - 10);
            value >>= 4;
        } while (value != 0);
    }
    else if (base == 10)
    {
        /* 十进制：32位除法是安全的 */
        do
        {
            if (p >= end)
                break;
            *p++ = '0' + (value % 10);
            value /= 10;
        } while (value != 0);
    }
    else
    {
        if (p < end)
            *p++ = '?';
    }

    *p = '\0';

    /* 反转字符串 */
    for (char *a = buf, *b = p - 1; a < b; a++, b--)
    {
        char tmp = *a;
        *a = *b;
        *b = tmp;
    }
}

/* 将有符号整数转换为字符串 */
static void vga_itoa(int value, char *buf, int base, size_t buf_size)
{
    if (buf_size == 0)
        return;

    char *p = buf;
    char *end = buf + buf_size - 1;
    (void)end; // 抑制警告
    (void)p;   // 抑制警告
    /* 只在十进制时处理负数 */
    int is_negative = 0;
    if (base == 10 && value < 0)
    {
        is_negative = 1;
        value = -value;
    }

    /* 转换为无符号数处理 */
    vga_utoa((uint32_t)value, buf, base, buf_size);

    /* 如果是负数，在前面添加负号 */
    if (is_negative && strlen(buf) + 1 < buf_size)
    {
        /* 将字符串右移一位 */
        size_t len = strlen(buf);
        for (size_t i = len; i > 0; i--)
        {
            buf[i] = buf[i - 1];
        }
        buf[0] = '-';
        buf[len + 1] = '\0';
    }
}

/* 人类可读的字节大小格式化（避免64位除法） */
static void vga_human_size(uint64_t bytes, char *buf, size_t buf_size)
{
    static const char units[] = "BKMGT";
    uint32_t scale = 0; /* 0=B, 1=K, 2=M, 3=G, 4=T */

    /* 找到合适的单位 */
    for (int i = 0; i < 4; i++)
    {
        if (bytes < (1ULL << (10 * (i + 1))))
            break;
        scale++;
    }

    /* 计算整数部分和小数部分（通过移位避免64位除法） */
    uint64_t whole = bytes >> (scale * 10); /* 相当于 bytes / (1024^scale) */

    /* 计算小数部分：((bytes % div) * 10) / div */
    uint64_t remainder = bytes & ((1ULL << (scale * 10)) - 1); /* bytes % (1024^scale) */
    uint64_t frac = 0;

    if (scale > 0 && remainder > 0)
    {
        /* 小数部分 = (余数 * 10) >> (scale * 10) */
        frac = (remainder * 10) >> (scale * 10);
    }

    /* 构建字符串 */
    char tmp[32];
    vga_ulltoa(whole, tmp, 10, sizeof(tmp));
    size_t len = strlen(tmp);

    if (len >= buf_size)
    {
        *buf = '\0';
        return;
    }

    memcpy(buf, tmp, len);

    /* 添加小数部分（如果有） */
    if (frac > 0 && len + 3 < buf_size)
    {
        buf[len] = '.';
        buf[len + 1] = '0' + (char)frac;
        buf[len + 2] = '\0';
        len += 2;
    }
    else
    {
        buf[len] = '\0';
    }

    /* 添加单位 */
    if (len + 2 <= buf_size)
    {
        buf[len] = units[scale];
        buf[len + 1] = '\0';
    }
}

/* 格式化输出到VGA屏幕 */
void vga_printf(const char *fmt, ...)
{
    if (!fmt)
        return;

    va_list args;
    va_start(args, fmt);

    char buf[64];

    for (const char *p = fmt; *p; p++)
    {
        if (*p != '%')
        {
            vga_putc(*p);
            continue;
        }

        p++; /* 跳过 '%' */

        if (*p == '\0')
            break;

        switch (*p)
        {
        case 'c':
        {
            char c = (char)va_arg(args, int);
            vga_putc(c);
            break;
        }
        case 's':
        {
            const char *s = va_arg(args, const char *);
            if (!s)
                s = "(null)";
            vga_write(s);
            break;
        }
        case 'd':
        case 'i':
        {
            int v = va_arg(args, int);
            vga_itoa(v, buf, 10, sizeof(buf));
            vga_write(buf);
            break;
        }
        case 'u':
        {
            uint32_t v = va_arg(args, uint32_t);
            vga_utoa(v, buf, 10, sizeof(buf));
            vga_write(buf);
            break;
        }
        case 'x':
        case 'X':
        {
            uint32_t v = va_arg(args, uint32_t);
            vga_utoa(v, buf, 16, sizeof(buf));
            vga_write(buf);
            break;
        }
        case 'h':
        {
            uint64_t v = va_arg(args, uint64_t);
            vga_human_size(v, buf, sizeof(buf));
            vga_write(buf);
            break;
        }
        case 'l':
        {
            p++; /* 跳过第一个 'l' */
            if (*p == '\0')
            {
                vga_putc('%');
                vga_putc('l');
                break;
            }

            if (*p == 'l')
            {
                /* %ll... 格式 */
                p++;
                if (*p == '\0')
                {
                    vga_write("%ll");
                    break;
                }

                if (*p == 'u' || *p == 'x' || *p == 'X')
                {
                    /* %llu 或 %llx：无符号64位 */
                    uint64_t v = va_arg(args, uint64_t);
                    vga_ulltoa(v, buf, (*p == 'u') ? 10 : 16, sizeof(buf));
                    vga_write(buf);
                }
                else if (*p == 'd' || *p == 'i')
                {
                    /* %lld 或 %lli：有符号64位 */
                    int64_t v = va_arg(args, int64_t);
                    if (v < 0)
                    {
                        vga_putc('-');
                        v = -v;
                    }
                    vga_ulltoa((uint64_t)v, buf, 10, sizeof(buf));
                    vga_write(buf);
                }
                else
                {
                    vga_write("%ll");
                    vga_putc(*p);
                }
            }
            else if (*p == 'u' || *p == 'x' || *p == 'X')
            {
                /* %lu 或 %lx：无符号32位 */
                uint32_t v = va_arg(args, uint32_t);
                vga_utoa(v, buf, (*p == 'u') ? 10 : 16, sizeof(buf));
                vga_write(buf);
            }
            else if (*p == 'd' || *p == 'i')
            {
                /* %ld 或 %li：有符号32位 */
                int32_t v = va_arg(args, int32_t);
                vga_itoa(v, buf, 10, sizeof(buf));
                vga_write(buf);
            }
            else
            {
                vga_putc('%');
                vga_putc('l');
                vga_putc(*p);
            }
            break;
        }
        case 'p':
        {
            /* 指针：以 0x 开头的8位十六进制 */
            uint32_t v = va_arg(args, uint32_t);
            vga_write("0x");
            vga_utoa(v, buf, 16, sizeof(buf));

            /* 左侧补0到8位 */
            int len = strlen(buf);
            for (int i = 0; i < 8 - len; ++i)
                vga_putc('0');
            vga_write(buf);
            break;
        }
        case '%':
            vga_putc('%');
            break;
        default:
            vga_putc('%');
            vga_putc(*p);
            break;
        }
    }

    va_end(args);
}

/*
 * 通过在屏幕上输出内容，让你能直观看到 vga_printf 是否工作正确
 */
void vga_test_printf(void)
{
    vga_clear();
    vga_set_color(VGA_WHITE_ON_BLACK);

    vga_write("==== vga_printf Test Start ====\n\n");

    /* 1. Basic format tests */
    vga_write("[1] Basic Types:\n");
    vga_printf("Char: %c\n", 'A');
    vga_printf("String: %s\n", "Hello VGA!");
    vga_printf("Signed %%d: %d\n", -12345);
    vga_printf("Unsigned %%u: %u\n", 12345);
    vga_printf("Hex %%x: %x\n", 0xdeadbeef);
    vga_printf("Percent: %%\n\n");

    /* 2. Pointer format test */
    vga_write("[2] Pointer %%p Test:\n");
    int dummy = 0x11112222;
    vga_printf("Pointer: %p\n\n", &dummy);

    /* 3. 64-bit integer tests */
    vga_write("[3] 64-bit Integer Test:\n");
    uint64_t big = 0x1234567887654321ULL;
    int64_t big_signed = -987654321012345LL;

    vga_printf("uint64 hex %%llx: %llx\n", big);
    vga_printf("uint64 dec %%llu: %llu\n", big);
    vga_printf("int64  dec %%lld: %lld\n\n", big_signed);

    /* 4. Custom human-readable size test (%h) */
    vga_write("[4] Custom %%h Test:\n");
    vga_printf("512 B: %h\n", 512ULL);
    vga_printf("4 KB: %h\n", 4096ULL);
    vga_printf("6 MB: %h\n", 6ULL * 1024 * 1024);
    vga_printf("3 GB: %h\n", 3ULL * 1024 * 1024 * 1024);
    vga_write("\n");

    /* 5. Control characters test */
    vga_write("[5] Control Chars:\n");
    vga_printf("TAB:\tEND\n");
    vga_printf("Backspace: ABC\bD (should be ABD)\n\n");

    vga_write("==== vga_printf Test End ====\n");
}