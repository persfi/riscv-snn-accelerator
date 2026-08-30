ENTRY( _start )

SECTIONS
{
  . = 0x00000000;

  .text.init : { *(.text.init) }      /* _start lands here, at 0x0 */
  .text      : { *(.text .text.*) }
  .rodata    : { *(.rodata .rodata.*) }
  .data      : { *(.data .data.*) }
  . = ALIGN(4);          /* round . up to a 4-byte boundary */
  _bss_start = .;   /* symbol = "the address right here", before .bss */
  .bss       : { *(.bss .bss.* COMMON) }
  . = ALIGN(4);
  _bss_end = .;          /* symbol = address just past .bss */
  
  _end = .;
  _stack_top = 0x4000;   /* benchmark only: weights sit above, at 0x4000 */   

  ASSERT(_end <= _stack_top - 0x100, "sections grew into the stack reserve")

}
