SECTIONS
{
  . = 0xffff000000000000 + 0x80000;
  .text : { *(.text) }
  . = ALIGN(8);
  .rodata : { *(.rodata) }
  . = ALIGN(8);
  .data : { *(.data) }
  . = ALIGN(8);
  __bss_start = .;
  .bss : { *(.bss) }
  __bss_end = .;
  __bss_size = __bss_end - __bss_start;
  . = ALIGN(8);
  __prepage_heap = .;
}
