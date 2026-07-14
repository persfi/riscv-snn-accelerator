#include "Vbranch.h"
#include "tb_harness.h"

constexpr uint8_t F3_BEQ = 0b000;
constexpr uint8_t F3_BNE = 0b001;
constexpr uint8_t F3_BLT = 0b100;
constexpr uint8_t F3_BGE = 0b101;
constexpr uint8_t F3_BLTU = 0b110;
constexpr uint8_t F3_BGEU = 0b111;

int main() {
  Testbench<Vbranch> tb("verif/build/branch/branch.vcd");
  auto& dut = tb.top;

  dut.branch_ctrl = 1;

  // beq
  dut.funct3 = F3_BEQ;
  dut.rs1_data = 2;
  dut.rs2_data = 2;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 1, "beq: equal operands should take the branch");
  dut.rs2_data = -4;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 0, "beq: unequal operands should not take the branch");

  // bne
  dut.funct3 = F3_BNE;
  dut.rs1_data = 2;
  dut.rs2_data = -4;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 1, "bne: unequal operands should take the branch");
  dut.rs2_data = 2;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 0, "bne: equal operands should not take the branch");

  // blt
  dut.funct3 = F3_BLT;
  dut.rs1_data = -1;
  dut.rs2_data = 0;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 1, "blt: signed -1 < 0 should take the branch");
  dut.rs1_data = -4;
  dut.rs2_data = -4;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 0,
           "blt: equal operands should not take the branch, proves '=' not ncluded");

  // bltu
  dut.funct3 = F3_BLTU;
  dut.rs1_data = -1;
  dut.rs2_data = 0;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 0,
           "bltu: unsigned -1 is huge, not < 0, proves sign/unsigned split against blt");
  dut.rs1_data = -4;
  dut.rs2_data = -4;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 0,
           "bltu: equal operands should not take the branch, proves '=' not included");

  // bge
  dut.funct3 = F3_BGE;
  dut.rs1_data = 2;
  dut.rs2_data = 2;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 1,
           "bge: equal operands should take the branch, proves '=' included");
  dut.rs1_data = -1;
  dut.rs2_data = 0;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 0,
           "bge: signed -1 < 0 should not take the branch, proves sign/unsigned split against bgeu");

  // bgeu
  dut.funct3 = F3_BGEU;
  dut.rs1_data = 2;
  dut.rs2_data = 2;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 1,
           "bgeu: equal operands should take the branch, proves '=' included");
  dut.rs1_data = -1;
  dut.rs2_data = 0;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 1,
           "bgeu: unsigned -1 is huge, >= 0, proves sign/unsigned split against bge");
  dut.rs1_data = 2;
  dut.rs2_data = 3;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 0, "bgeu: 2 is not >= 3, should not take the branch");

  
  
  dut.funct3 = 0b010; //not a branch encoding, should hit the default case
  dut.rs1_data = 2;
  dut.rs2_data = 2;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 0,
           "should hit the default case, branch_taken=0");

  // branch_ctrl=0:  should override 0 even when other conditions are met
  dut.branch_ctrl = 0;
  dut.funct3 = F3_BEQ;
  dut.rs1_data = 2;
  dut.rs2_data = 2;
  tb.settle();
  CHECK_EQ(dut.branch_taken, 0,
           "branch_ctrl=0 should suppress branch_taken even though beq's operands are equal");

  return tb_report();
}
