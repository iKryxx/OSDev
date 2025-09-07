OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
  /* Load the kernel at 1 MiB, but we’ll run it in higher half later; for now identity is fine */
  . = 1M;
  _kernel_start = .;

  .text : ALIGN(4K) {
    KEEP(*(.multiboot2))
    *(.text .text.*)
  }

  .rodata : ALIGN(4K) { *(.rodata .rodata.*) }
  .data : ALIGN(4K)   { *(.data .data.*) }
  .bss : ALIGN(4K)    { *(COMMON) *(.bss .bss.*) }

  _kernel_end = .;
}
