#include "vga.h"
#include "gdt.h"
#include "interrupt.h"
#include "timer.h"
#include "boot_info.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"

void main()
{
  vga_clear();
  vga_printf("Hello, kernel world!\n");
  gdt_init();
  idt_init();
  enable_interrupts();
  init_timer(TIMER_FREQUENCY);
  boot_info_init();
  // vga_printf("boot_info.e820_map: %p\n", &boot_info.e820_map);
  // boot_info_dump();
  pmm_init(&boot_info);
  vmm_init();
  init_kheap();

  // 用随机、碎片化、高频率的分配-释放序列反复测试堆分配器，若失败则会立即 PANIC
  kheap_killer();

  while (1)
    ;
}
