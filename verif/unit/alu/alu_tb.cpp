#include "Valu.h"
#include "tb_harness.h"

constexpr uint8_t ALU_ADD  = 0b0000;
constexpr uint8_t ALU_SUB  = 0b1000;
constexpr uint8_t ALU_SLL  = 0b0001;
constexpr uint8_t ALU_SLT  = 0b0010;
constexpr uint8_t ALU_SLTU = 0b0011;
constexpr uint8_t ALU_XOR  = 0b0100;
constexpr uint8_t ALU_SRL  = 0b0101;
constexpr uint8_t ALU_SRA  = 0b1101;
constexpr uint8_t ALU_OR   = 0b0110;
constexpr uint8_t ALU_AND  = 0b0111;

int main() {
    Testbench<Valu> tb("verif/build/alu/alu.vcd");
    auto& dut = tb.top;

    dut.a = 0x00000006;
    dut.b = 0x00000001;
    dut.alu_ctrl = 0b1111;
    tb.settle();
    CHECK_EQ(dut.result,0x00000000,"undefined operations should produce default 0");

    //alu_add
    
    dut.alu_ctrl = ALU_ADD;
    tb.settle();
    CHECK_EQ(dut.result,0x00000007,"alu_add should produce correct add result");
    dut.a = 0xffffffff;
    tb.settle();
    CHECK_EQ(dut.result,0x00000000,"alu_add should produce correct add result at max boundary");
    dut.a = 0x00000000;
    dut.b = 0x00000000;
    tb.settle();
    CHECK_EQ(dut.result,0x00000000,"alu_add should produce correct add result at min boundary");

    //alu_sub
    dut.a = 0x00000006;
    dut.b = 0x00000003;
    dut.alu_ctrl = ALU_SUB;
    tb.settle();
    CHECK_EQ(dut.result,0x00000003,"alu_sub should produce correct sub result");
    dut.a = 0x00000006;
    dut.b = 0x00000007;
    tb.settle();
    CHECK_EQ(dut.result,0xffffffff,"alu_sub should produce correct sub result at boundary");

    //alu_sll
    dut.a = 1;
    dut.b = 32;
    dut.alu_ctrl = ALU_SLL;
    tb.settle();
    CHECK_EQ(dut.result,0x00000001,"alu_sll should mask b>31");
    dut.b = 0;
    tb.settle();
    CHECK_EQ(dut.result,0x00000001,"alu_sll should not shift");
    dut.b = 2;
    tb.settle();
    CHECK_EQ(dut.result,0x00000004,"alu_sll should shift according to b");
    dut.b = 31;
    tb.settle();
    CHECK_EQ(dut.result,0x80000000,"alu_sll should shift according to b at boundary");

    //alu_slt
    dut.a = 0x80000000;//most negative
    dut.b =0x7FFFFFFF;//most positive
    dut.alu_ctrl = ALU_SLT;
    tb.settle();
    CHECK_EQ(dut.result,0x00000001,"alu_slt should compare signed values with min/max");
    dut.a = -1;
    dut.b = 1;                     
    tb.settle();
    CHECK_EQ(dut.result, 0x00000001, "alu_slt should compare signed values (paired with sltu)");
    dut.a = 1;
    dut.b = 1;
    tb.settle();
    CHECK_EQ(dut.result,0x00000000,"alu_slt should compute 0 with the same a,b");
    dut.b = 0; //a=1
    tb.settle();
    CHECK_EQ(dut.result,0x00000000,"alu_slt should compare a>b");

    //alu_sltu
    dut.a = -1;
    dut.b = 1;
    dut.alu_ctrl = ALU_SLTU;
    tb.settle();
    CHECK_EQ(dut.result,0x00000000,"alu_sltu should compared unsigned");
    dut.b = 0; 
    tb.settle();
    CHECK_EQ(dut.result,0x00000000,"alu_sltu should compare a>b and output 0");
    dut.a = 0;
    dut.b = 1;
    tb.settle();
    CHECK_EQ(dut.result,0x00000001,"alu_sltu should compare a<b and output 1");

    //alu_xor
    dut.a = 0x00000000;
    dut.b = 0xffffffff;
    dut.alu_ctrl = ALU_XOR;
    tb.settle();
    CHECK_EQ(dut.result,0xffffffff,"alu_xor should work at boundaries");
    dut.a = 0xffffffff;
    tb.settle();
    CHECK_EQ(dut.result,0x00000000,"alu_xor should return 0 when every bits are the same");

    //alu_or
    dut.a = 0x00000000;
    dut.b = 0xffffffff;
    dut.alu_ctrl = ALU_OR;
    tb.settle();
    CHECK_EQ(dut.result,0xffffffff,"alu_or should work at boundaries");
    dut.a = 0xffffffff;
    tb.settle();
    CHECK_EQ(dut.result,0xffffffff,"alu_or should return 1 for every bit");

    //alu_and
    dut.a = 0x00000000;
    dut.b = 0xffffffff;
    dut.alu_ctrl = ALU_AND;
    tb.settle();
    CHECK_EQ(dut.result,0x00000000,"alu_and should work at boundaries");
    dut.a = 0xffffffff;
    tb.settle();
    CHECK_EQ(dut.result,0xffffffff,"alu_and should return 1 when both bits are 1");

    //alu_srl
    dut.a = 0x00000002;
    dut.b = 32;
    dut.alu_ctrl = ALU_SRL;
    tb.settle();
    CHECK_EQ(dut.result,0x00000002,"alu_srl should mask b>31");
    dut.b = 0;
    tb.settle();
    CHECK_EQ(dut.result,0x00000002,"alu_srl should not shift");
    dut.b = 1;
    tb.settle();
    CHECK_EQ(dut.result,0x00000001,"alu_srl should shift according to b");
    dut.a = 0x80000001;
    dut.b = 1;
    tb.settle();
    CHECK_EQ(dut.result,0x40000000,"alu_srl should shift logically (zero-fill), not arithmetically");

    //alu_sra
    dut.a = 0x80000001;
    dut.b = 32;
    dut.alu_ctrl = ALU_SRA;
    tb.settle();
    CHECK_EQ(dut.result,0x80000001,"alu_sra should mask b>31");
    dut.b = 0;
    tb.settle();
    CHECK_EQ(dut.result,0x80000001,"alu_sra should not shift");
    dut.b = 1;
    tb.settle();
    CHECK_EQ(dut.result,0xc0000000,"alu_sra should shift and preserve signs according to b");


    return tb_report();
}
