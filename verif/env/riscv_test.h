// Minimal bare-metal test environment for running the official riscv-tests
// rv32ui vectors on a base-RV32I core that has no CSRs, traps, or privilege
// modes.
//
// This substitutes for the submodule's env/p/riscv_test.h, which assumes a
// machine-mode core (mtvec/mcause/ecall). We keep the upstream *test bodies*
// (verif/riscv-tests/isa/rv32ui/*.S) completely untouched and replace only the
// boot + pass/fail scaffolding. It is selected purely by include order: the
// build passes -Iverif/env before anything else, so each test's
// #include "riscv_test.h" resolves to this file instead of env/p's.
//
// Pass/fail protocol (watched by the Verilator harness, then it ends the sim):
//   tohost == 1          -> every sub-test passed
//   tohost == (n<<1)|1   -> sub-test n failed
//
// Every instruction used below (addi/li, lui, slli, ori, auipc/la, sw, jal/j)
// is in the base RV32I set the core already decodes -- no fence, no ecall, no
// csr. See docs/DESIGN.md for the exclusion + minimal-env rationale.

#ifndef RISCV_TEST_MINIMAL_H
#define RISCV_TEST_MINIMAL_H

// XLEN markers the test bodies invoke; no setup needed on a bare RV32I core.
#define RVTEST_RV32U
#define RVTEST_RV64U
#define RVTEST_RV32M
#define RVTEST_RV64M

// The tests carry the current sub-test number in gp and check against it.
#define TESTNUM gp

// clang-format off
// The macros below expand to RISC-V assembly. Do NOT let a C formatter reflow
// them: it collapses ".section .text.init" -> ".section.text.init" (directive
// glued to operand), which the assembler rejects as an unknown pseudo-op.
//------------------------------------------------------------------------------
// Boot
//------------------------------------------------------------------------------
// _start must sit at the core's reset vector (0x0). The linker script lists
// .text.init first, and _start is its first symbol, so _start == 0x0. No
// register/stack init is needed: each sub-test loads its own operands and sets
// TESTNUM itself before using them.
#define RVTEST_CODE_BEGIN \
  .section .text.init;     \
  .globl _start;          \
  _start:

#define RVTEST_CODE_END

//------------------------------------------------------------------------------
// Pass / Fail
//------------------------------------------------------------------------------
// Reached by falling through all sub-tests: report 1 and spin. The core has no
// halt; the spin parks it harmlessly until the harness sees tohost and stops.
#define RVTEST_PASS  \
  li TESTNUM, 1;     \
  la t0, tohost;     \
  sw TESTNUM, 0(t0); \
  1 : j 1b;

// Reached by a sub-test's failing branch (gp holds its number): report
// (n<<1)|1 and spin.
#define RVTEST_FAIL         \
  slli TESTNUM, TESTNUM, 1; \
  ori TESTNUM, TESTNUM, 1;  \
  la t0, tohost;            \
  sw TESTNUM, 0(t0);        \
  1 : j 1b;

//------------------------------------------------------------------------------
// Data
//------------------------------------------------------------------------------
// The tohost/fromhost mailbox lives in its own section; the linker script
// places .tohost after .text/.data, so its address falls out of the code+data
// size and can never collide. begin/end_signature bound any data some tests
// emit.
#define RVTEST_DATA_BEGIN               \
  .pushsection .tohost, "aw", @progbits; \
  .align 6;                             \
  .global tohost;                       \
  tohost:                               \
  .dword 0;                             \
  .align 6;                             \
  .global fromhost;                     \
  fromhost:                             \
  .dword 0;                             \
  .popsection;                          \
  .align 4;                             \
  .global begin_signature;              \
  begin_signature:

#define RVTEST_DATA_END  \
  .align 4;              \
  .global end_signature; \
  end_signature:
// clang-format on

#endif  // RISCV_TEST_MINIMAL_H
