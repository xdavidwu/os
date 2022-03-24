SECTIONS
{
  . = 0x100000;
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
}
