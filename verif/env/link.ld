/* Linker script for running riscv-tests rv32ui vectors on the core.
 *
 * Bases the program at 0x0 to match the core's reset vector (RESET_VECTOR in
 * rtl/core/defs.vh); the upstream env/p/link.ld uses 0x80000000 for spike.
 * .tohost is placed AFTER .text/.data so its address is whatever comes next --
 * collision-proof, no guessed address. The build reads tohost's resolved
 * address out of the ELF (nm) and hands it to the harness.
 *
 * Note: this only assigns addresses; nothing here knows the RTL's imem/dmem
 * DEPTH. Keeping every symbol below DEPTH words is a design responsibility, not
 * something the linker checks (see docs/DESIGN.md).
 */

OUTPUT_ARCH( "riscv" )
ENTRY( _start )

SECTIONS
{
  . = 0x00000000;

  .text.init : { *(.text.init) }      /* _start lands here, at 0x0 */
  .text      : { *(.text .text.*) }
  .rodata    : { *(.rodata .rodata.*) }
  .data      : { *(.data .data.*) }
  .tohost    : { *(.tohost) }         /* placed after code+data */
  .bss       : { *(.bss .bss.* COMMON) }

  _end = .;
}
