#include <stdarg.h>
#include "vga.h"
#include "io.h"
#include "ports.h"

static uint16_t *const VGA_BUF = (uint16_t *)VGA_MEMORY;
static uint8_t cursor_row = 0;
static uint8_t cursor_col = 0;
static uint8_t current_color = VGA_WHITE_ON_BLACK;

/* 同步硬件光标位置 */
static void vga_sync_cursor()
{
    // The screen is 80 characters wide.
    uint16_t cursorLocation = cursor_row * 80 + cursor_col;
    outb(VGA_CRTC_ADDR, VGA_CRTC_CURSOR_LOC_HI); // Tell the VGA board we are setting the high cursor byte.
    outb(VGA_CRTC_DATA, cursorLocation >> 8);   // Send the high cursor byte.
    outb(VGA_CRTC_ADDR, VGA_CRTC_CURSOR_LOC_LO); // Tell the VGA board we are setting the low cursor byte.
    outb(VGA_CRTC_DATA, cursorLocation);        // Send the low cursor byte.
}

/* 手动移动光标位置 */
void vga_move_cursor(uint8_t row, uint8_t col)
{
    if (row < VGA_HEIGHT && col < VGA_WIDTH)
    {
        cursor_row = row;
        cursor_col = col;
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

/* 输出单个字符（自动换行） */
void vga_putc(char c)
{
    if (c == '\n')
    {
        cursor_row++;
        cursor_col = 0;
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

    /* 滚屏 */
    if (cursor_row >= VGA_HEIGHT)
    {
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++)
        {
            VGA_BUF[i] = VGA_BUF[i + VGA_WIDTH];
        }
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

/* 将整数转换为字符串（支持十进制和十六进制） */
static void vga_itoa(int value, char *buf, int base, size_t buf_size)
{
    // 添加参数 buf_size 来接收缓冲区大小
    char *p = buf;
    char *buf_end = buf + buf_size - 1; // 指向buf的最后一个可用位置

    if (buf_size == 0)
        return; // 缓冲区大小为0，直接返回

    unsigned int v = (base == 10 && value < 0) ? -value : value;
    do
    {
        // 关键修复：在写入前检查是否超出缓冲区边界
        if (p >= buf_end)
        {
            break; // 如果即将溢出，就停止转换
        }
        int digit = v % base;
        *p++ = (digit < 10) ? '0' + digit : 'a' + (digit - 10);
        v /= base;
    } while (v);

    if (base == 10 && value < 0)
    {
        // 同样需要检查负号的位置
        if (p < buf_end)
        {
            *p++ = '-';
        }
    }
    *p = '\0';

    // 反转字符串
    for (char *a = buf, *b = p - 1; a < b; a++, b--)
    {
        char tmp = *a;
        *a = *b;
        *b = tmp;
    }
}
/* 格式化输出到 VGA 屏幕 */
void vga_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char buf[64]; // 缓冲区大小为64

    for (const char *p = fmt; *p; p++)
    {
        if (*p != '%')
        {
            vga_putc(*p);
            continue;
        }

        p++; // 跳过 %

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
        case 'x':
        case 'X':
        {
            int v = va_arg(args, int);
            vga_itoa(v, buf, 16, sizeof(buf));
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