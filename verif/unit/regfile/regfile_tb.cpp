#include "Vregfile.h"
#include "tb_harness.h"

int main() {
    Testbench<Vregfile> tb("verif/build/regfile/regfile.vcd");
    auto& dut = tb.top;

    //can regfile read rs1
    dut.rd_we = 1;
    dut.rd_addr = 5;
    dut.rd_data = 0x00000010;
    dut.rs1_addr = 5;
    tb.tick();
    CHECK_EQ(dut.rs1_data, 0x00000010, "reads rs1_data(x5) properly");
    dut.rd_we = 0;
    dut.rd_addr = 5;
    dut.rd_data = 0x00000015;
    tb.tick();
    CHECK_EQ(dut.rs1_data, 0x00000010, "rd_we = 0 should not change anything");

    //can regfile read rs2 at boundary
    dut.rd_we = 1;
    dut.rd_addr = 31;
    dut.rd_data = 0x00000011;
    dut.rs2_addr = 31;
    tb.tick();
    CHECK_EQ(dut.rs2_data, 0x00000011, "reads rs2_data(x31) properly and x31 boundary works");
    CHECK_EQ(dut.rs1_data, 0x00000010, "can rs1(x5) and rs2(x31) read correctly at the same time"); //if the first two tests pass, rs1_data should still be 0x00000010
    

    //can x0 remain 0 after a non zero write
    dut.rd_we = 1;
    dut.rd_addr = 0;
    dut.rd_data = 0x23456789;
    dut.rs2_addr=0;
    dut.rs1_addr = 0;
    tb.tick();
    CHECK_EQ(dut.rs2_data, 0, "x0 is always 0 for rs2");
    CHECK_EQ(dut.rs1_data, 0, "x0 is always 0 for rs1");

    //does it write as posedge
    dut.rd_we = 1;
    dut.rd_addr = 5;
    dut.rd_data = 0xabababab;
    dut.rs1_addr = 5; //if read rs1 data passes the first check, rs1 should still remain 0x00000010 after negedge
    tb.negedge();
    CHECK_EQ(dut.rs1_data, 0x00000010, "rd unwritten at negedge");
    tb.posedge();
    CHECK_EQ(dut.rs1_data, 0xabababab, "rd writes at posedge");
    

    return tb_report();
}
