// Instruction encodings and expected immediates are derived from
// verif/unit/imm_gen/vectors.s (assembled via `make dump-asm
// FILE=verif/unit/imm_gen/vectors.s`); this file just feeds those words
// through the DUT and checks the decoded immediate.
#include "Vimm_gen.h"
#include "tb_harness.h"

constexpr uint8_t IMM_I = 0b000;
constexpr uint8_t IMM_S = 0b010;
constexpr uint8_t IMM_B = 0b110;
constexpr uint8_t IMM_U = 0b111;
constexpr uint8_t IMM_J = 0b001;

int main() {
    Testbench<Vimm_gen> tb("verif/build/imm_gen/imm_gen.vcd");
    auto& dut = tb.top;

    // I type
    dut.imm_src = IMM_I;
    dut.inst = 0x80000293; // addi x5, x0, -2048
    tb.settle();
    CHECK_EQ(dut.imm, 0xfffff800, "imm_gen I-type should sign-extend min imm");
    dut.inst = 0x7ff00293; // addi x5, x0, 2047
    tb.settle();
    CHECK_EQ(dut.imm, 0x000007ff, "imm_gen I-type should produce max imm");
    dut.inst = 0x00300293; // addi x5, x0, 3
    tb.settle();
    CHECK_EQ(dut.imm, 0x00000003, "imm_gen I-type should produce positive imm");
    dut.inst = 0xffd00293; // addi x5, x0, -3
    tb.settle();
    CHECK_EQ(dut.imm, 0xfffffffd, "imm_gen I-type should sign-extend negative imm");

    // S type
    dut.imm_src = IMM_S;
    dut.inst = 0x80a2a023; // sw x10, -2048(x5)
    tb.settle();
    CHECK_EQ(dut.imm, 0xfffff800, "imm_gen S-type should sign-extend min imm");
    dut.inst = 0x7ea2afa3; // sw x10, 2047(x5)
    tb.settle();
    CHECK_EQ(dut.imm, 0x000007ff, "imm_gen S-type should produce max imm");
    dut.inst = 0x00a2a3a3; // sw x10, 7(x5)
    tb.settle();
    CHECK_EQ(dut.imm, 0x00000007, "imm_gen S-type should reassemble split positive imm");
    dut.inst = 0xfea2aca3; // sw x10, -7(x5)
    tb.settle();
    CHECK_EQ(dut.imm, 0xfffffff9, "imm_gen S-type should reassemble split negative imm");

    // B type
    dut.imm_src = IMM_B;
    dut.inst = 0x00a28263; // beq x5, x10, fwd (back -> fwd, +4)
    tb.settle();
    CHECK_EQ(dut.imm, 0x00000004, "imm_gen B-type should produce positive branch offset");
    dut.inst = 0xfea28ee3; // beq x5, x10, back (fwd -> back, -4)
    tb.settle();
    CHECK_EQ(dut.imm, 0xfffffffc, "imm_gen B-type should produce negative branch offset");

    // U type
    dut.imm_src = IMM_U;
    dut.inst = 0xabcde2b7; // lui x5, 0xabcde
    tb.settle();
    CHECK_EQ(dut.imm, 0xabcde000, "imm_gen U-type should place imm in upper 20 bits");
    dut.inst = 0x800002b7; // lui x5, 0x80000
    tb.settle();
    CHECK_EQ(dut.imm, 0x80000000, "imm_gen U-type should handle top bit set");

    // J type
    dut.imm_src = IMM_J;
    dut.inst = 0x004002ef; // jal x5, fwd2 (back2 -> fwd2, +4)
    tb.settle();
    CHECK_EQ(dut.imm, 0x00000004, "imm_gen J-type should produce positive jump offset");
    dut.inst = 0xffdff2ef; // jal x5, back2 (fwd2 -> back2, -4)
    tb.settle();
    CHECK_EQ(dut.imm, 0xfffffffc, "imm_gen J-type should produce negative jump offset");

    return tb_report();
}
