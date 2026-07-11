#include "Vcore.h"
#include "Vcore___024root.h"
#include "tb_harness.h"

int main() {
    Testbench<Vcore> tb("verif/build/core/core.vcd");
    auto& dut = tb.top;
    load_hex(tb.top.rootp->core__DOT__imem__DOT__mem, "verif/unit/core/vectors.hex", 1024); //imem is now nested in core so two DOTS

    dut.rst=1;
    tb.tick();
    TRACE_LINE("cycle=%llu pc=%08x inst=%08x",
           (unsigned long long)tb.cycle(), (uint32_t)dut.pc_q, (uint32_t)dut.inst);
    CHECK_EQ(dut.pc_q, 0x00000000, "pc_q should reset to 0 after rst=1");
    CHECK_EQ(dut.inst, 0x00000008, "imem should read inst addr 0x0");

    dut.rst=0;
    tb.tick();
    TRACE_LINE("cycle=%llu pc=%08x inst=%08x",
           (unsigned long long)tb.cycle(), (uint32_t)dut.pc_q, (uint32_t)dut.inst);
    CHECK_EQ(dut.pc_q, 0x00000004, "pc_q should advance 4 after every clk tick");
    CHECK_EQ(dut.inst, 0x00000000, "imem should read inst at addr pc_q");

    tb.tick();
    TRACE_LINE("cycle=%llu pc=%08x inst=%08x",
           (unsigned long long)tb.cycle(), (uint32_t)dut.pc_q, (uint32_t)dut.inst);
    CHECK_EQ(dut.pc_q, 0x00000008, "pc_q should advance 4 after every clk tick");
    CHECK_EQ(dut.inst, 0x00000015, "imem should read inst at addr pc_q");
    
    dut.rst=1;
    tb.tick();
    TRACE_LINE("cycle=%llu pc=%08x inst=%08x",
           (unsigned long long)tb.cycle(), (uint32_t)dut.pc_q, (uint32_t)dut.inst);
    CHECK_EQ(dut.pc_q, 0x00000000, "pc_q should reset to 0 after rst=1 in mid cycle");
    CHECK_EQ(dut.inst, 0x00000008, "imem should read inst addr 0x0");


    return tb_report();
}
