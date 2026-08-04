#include "Vcounter.h"
#include "tb_harness.h"

static void reset(Testbench<Vcounter>& tb) {
    tb.top.rst   = 1;
    tb.top.stall = 0;
    tb.tick();
    tb.top.rst = 0;
}

static void tick_n(Testbench<Vcounter>& tb, int n) {
    for (int i = 0; i < n; i++) tb.tick();
}

int main() {
    Testbench<Vcounter> tb("verif/build/counter/counter.vcd");
    auto& dut = tb.top;

    dut.cnt_limit = 31;
    dut.stall     = 0;
    reset(tb);

    // counts up on consecutive ticks
    tick_n(tb, 3);
    CHECK_EQ(dut.q, 3, "q should count up one per tick");
    dut.stall     = 1;
    tick_n(tb, 3);
    CHECK_EQ(dut.q, 3, "q should stall");
    dut.stall     = 0;

    // wraps at cnt_limit, not at the width's maximum
    tick_n(tb, 28);                       // 3 -> 31
    CHECK_EQ(dut.q, 31, "q should reach cnt_limit");
    tb.tick();
    CHECK_EQ(dut.q, 0, "q should wrap to 0 the tick after reaching cnt_limit");

    // stall holds
    tick_n(tb, 31);                   
    dut.stall = 1;
    tick_n(tb, 2);
    CHECK_EQ(dut.q, 31, "stall should hold q even when q == cnt_limit");
    dut.stall = 0;

    // reset wins over stall
    reset(tb);
    tick_n(tb, 2);
    CHECK_EQ(dut.q, 2, "q should be 2");
    dut.stall = 1;
    dut.rst   = 1;
    tb.tick();
    CHECK_EQ(dut.q, 0, "rst must win over stall");
    dut.rst   = 0;
    dut.stall = 0;

    
    dut.cnt_limit = 0;
    reset(tb);
    tick_n(tb, 3);
    CHECK_EQ(dut.q, 0, "cnt_limit=0 should hold q at 0");

    TRACE_LINE("cycles=%llu", (unsigned long long)tb.cycle());
    return tb_report();
}
