// pc.v has no adder of its own: pc_next is driven by an upstream mux that doesn't exist yet, so this TB plays that role: it feeds pc_q + 4 back in as pc_next before each tick to stand in for the pc+4 path.

#include "Vpc.h"
#include "tb_harness.h"

constexpr uint32_t RESET_VECTOR = 0x00000000;

int main() {
    Testbench<Vpc> tb("verif/build/pc/pc.vcd");
    auto& dut = tb.top;

    //reset lands on RESET_VECTOR regardless of pc_next
    dut.rst = 1;
    dut.pc_next = 0xabcdef00;
    tb.tick();
    CHECK_EQ(dut.pc_q, RESET_VECTOR, "reset should load RESET_VECTOR");

    //advances by 4 per posedge 
    dut.rst = 0;
    for (int i = 1; i <= 3; ++i) {
        dut.pc_next = dut.pc_q + 4;
        tb.tick();
        CHECK_EQ(dut.pc_q, RESET_VECTOR + 4*i, "pc_q should advance by 4 per posedge");
    }

    //reset mid-run returns to RESET_VECTOR (not just init)
    dut.rst = 1;
    tb.tick();
    CHECK_EQ(dut.pc_q, RESET_VECTOR, "reset mid-run should return to RESET_VECTOR");

    dut.rst = 0;
    dut.pc_next = dut.pc_q + 4;
    tb.tick();
    CHECK_EQ(dut.pc_q, RESET_VECTOR + 4, "pc_q should advance by 4 from RESET_VECTOR after mid-run reset");

    return tb_report();
}
