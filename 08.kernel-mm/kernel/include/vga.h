#ifndef KERNEL_VGA_H
#define KERNEL_VGA_H

#include "types.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t *)0xC00B8000)

/* 颜色属性宏 */
#define VGA_COLOR(fg, bg) ((bg << 4) | (fg))
#define VGA_WHITE_ON_BLACK VGA_COLOR(15, 0)

/* VGA 相关函数接口 */
void vga_clear(void);
void vga_putc(char c);
void vga_write(const char *s);
void vga_set_color(uint8_t color);
void vga_move_cursor(uint8_t row, uint8_t col);
void vga_printf(const char *fmt, ...);

// 单元测试
void vga_test_printf(void);
#endif