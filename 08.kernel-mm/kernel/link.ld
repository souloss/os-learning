ENTRY(main)

KERNEL_VMA  = 0xC0800000;         /* 内核虚拟基址 */
KERNEL_LMA  = 0x00200000;         /* boot loader 加载物理地址 */
PAGE_SIZE   = 0x1000;

SECTIONS
{
  /* 1. 提供“物理-虚拟”偏移常量 */
  kernel_phys_offset = KERNEL_LMA;
  kernel_virt_base   = KERNEL_VMA;

  /* 2. 文本段：虚拟运行地址 = 0xC0800000，物理加载 = 0x100000 */
  .text KERNEL_VMA : AT(KERNEL_LMA)
  {
      kernel_text_start = .;
      *(.text .text.* .gnu.linkonce.t.*)
      kernel_text_end = .;
  }

  /* 3. 只读数据 */
  .rodata ALIGN(PAGE_SIZE) :
  {
      kernel_rodata_start = .;
      *(.rodata .rodata.* .gnu.linkonce.r.*)
      kernel_rodata_end = .;
  }

  /* 4. 已初始化数据 */
  .data ALIGN(PAGE_SIZE) :
  {
      kernel_data_start = .;
      *(.data .data.* .gnu.linkonce.d.*)
      kernel_data_end = .;
  }

  /* 5. BSS（未初始化）*/
  .bss ALIGN(PAGE_SIZE) :
  {
      kernel_bss_start = .;
      *(.bss .bss.* .gnu.linkonce.b.*)
      . = ALIGN(PAGE_SIZE);
      kernel_bss_end = .;
  }

  kernel_end = .;
  kernel_size = kernel_end - KERNEL_VMA;   /* 总字节数 */
}
