#include "vga.h"
#include "gdt.h"
#include "interrupt.h"
#include "timer.h"

void main()
{
  volatile uint32_t *stack_canary = (volatile uint32_t *)0xF0000000 - 1;
  *stack_canary = 0xDEADBEEF;

  vga_clear();
  vga_printf("Hello, kernel world!\n");
  vga_printf("Value: %d, Hex: 0x%x, Char: %c, String: %s\n", 1234, 1234, 'A', "VGA printf OK!");
  gdt_init();
  idt_init();
  enable_interrupts();
  init_timer(TIMER_FREQUENCY);
  while (1)
    ;
}