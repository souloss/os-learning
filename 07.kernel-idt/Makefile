CHAPTER := 07.kernel-idt
MODULES := mbr loader kernel

mbr_SRC := boot/mbr.S
mbr_OFFSET := 0

loader_SRC := boot/loader.S
loader_OFFSET := 1

kernel_SRC := kernel
kernel_LDFILE := kernel/link.ld
kernel_OFFSET := 9

include ../Makefile.inc
