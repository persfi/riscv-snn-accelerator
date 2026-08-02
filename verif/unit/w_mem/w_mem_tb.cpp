#include "Vw_mem.h"
#include "Vw_mem___024root.h"
#include "tb_harness.h"

// One write cycle
static void write_word(Testbench<Vw_mem>& tb, uint32_t addr, uint32_t data) {
    tb.top.we    = 1;
    tb.top.addr  = addr;
    tb.top.wdata = data;
    tb.tick();
    tb.top.we = 0;
}

//Present an address and let one clock pass so the registered output updates.
static uint32_t read_word(Testbench<Vw_mem>& tb, uint32_t addr) {
    tb.top.addr = addr;
    tb.tick();
    return (uint32_t)tb.top.rdata;
}

int main() {
    Testbench<Vw_mem> tb("verif/build/w_mem/w_mem.vcd");
    auto& dut = tb.top;
    auto& mem = tb.top.rootp->w_mem__DOT__mem;   // poke/peek the array directly

    dut.we    = 0;
    dut.addr  = 0;
    dut.wdata = 0;
    tb.settle();

    // write word 0xacbd1234 to 25087 (we=1), the top of the address range
    write_word(tb, 25087, 0xacbd1234);
    CHECK_EQ(mem[25087], 0xacbd1234u, "we=1 should write the array at DEPTH-1");
    CHECK_EQ(read_word(tb, 25087), 0xacbd1234u, "read should return what was written");

    // write word 0x12341234 to 0 with we=0: nothing should be written
    dut.we    = 0;
    dut.addr  = 0;
    dut.wdata = 0x12341234;
    tb.tick();
    CHECK_EQ(mem[0], 0x00000000u, "we=0 should leave the array untouched");

    // same write with we=1: the array should change on the clock edge
    dut.we    = 1;
    dut.addr  = 0;
    dut.wdata = 0x12341234;
    tb.settle();
    CHECK_EQ(mem[0], 0x00000000u, "write should not land before the posedge");
    tb.tick();
    CHECK_EQ(mem[0], 0x12341234u, "write should land on the posedge");
    dut.we = 0;

    // read latency: rdata follows addr by one clock, not combinationally
    dut.addr = 25087;
    tb.tick();
    CHECK_EQ(dut.rdata, 0xacbd1234u, "rdata should hold mem[25087] after one tick");

    dut.addr = 0;
    tb.settle();
    CHECK_EQ(dut.rdata, 0xacbd1234u, "rdata should still hold the old value before the posedge");
    tb.tick();
    CHECK_EQ(dut.rdata, 0x12341234u, "rdata should follow addr one clock later");

    TRACE_LINE("cycles=%llu", (unsigned long long)tb.cycle());
    return tb_report();
}
