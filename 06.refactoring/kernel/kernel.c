#include "vga.h"

void main()
{
vga_clear();
vga_printf("Hello, kernel world!\n");
vga_printf("Value: %d, Hex: 0x%x, Char: %c, String: %s\n", 1234, 1234, 'A', "VGA printf OK!");
  while (1)
    ;
}