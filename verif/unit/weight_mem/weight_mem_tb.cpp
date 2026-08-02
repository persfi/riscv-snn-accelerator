#include "Vweight_mem.h"
#include "Vweight_mem___024root.h"
#include "tb_harness.h"

static void write_w1(Testbench<Vweight_mem>& tb, uint32_t addr, uint32_t data) {
    tb.top.w1_we       = 1;
    tb.top.w2_we       = 0;
    tb.top.weight_addr = addr;
    tb.top.weight_wdata = data;
    tb.tick();
    tb.top.w1_we = 0;
}

static void write_w2(Testbench<Vweight_mem>& tb, uint32_t addr, uint32_t data) {
    tb.top.w1_we       = 0;
    tb.top.w2_we       = 1;
    tb.top.weight_addr = addr;
    tb.top.weight_wdata = data;
    tb.tick();
    tb.top.w2_we = 0;
}

// layer = 0 selects w1, 1 selects w2. One tick for the registered read.
static uint32_t read_at(Testbench<Vweight_mem>& tb, uint32_t addr, int layer) {
    tb.top.weight_addr  = addr;
    tb.top.layer_state  = layer;
    tb.tick();
    return (uint32_t)tb.top.weight_rdata;
}

int main() {
    Testbench<Vweight_mem> tb("verif/build/weight_mem/weight_mem.vcd");
    auto& dut = tb.top;
    auto& w1_mem = tb.top.rootp->weight_mem__DOT__w1__DOT__mem;
    auto& w2_mem = tb.top.rootp->weight_mem__DOT__w2__DOT__mem;

    dut.w1_we        = 0;
    dut.w2_we        = 0;
    dut.weight_addr  = 0;
    dut.weight_wdata = 0;
    dut.layer_state  = 0;
    tb.settle();

    write_w1(tb, 512, 0xaaa);
    CHECK_EQ(w1_mem[512], 0xaaau, "w1_we should write w1 at addr 512");

    // both instances at index 0, written independently
    write_w1(tb, 0, 0xccc);
    write_w2(tb, 0, 0xbbb);
    CHECK_EQ(w2_mem[0], 0xbbbu, "w2_we should write w2 at addr 0");
    CHECK_EQ(w1_mem[0], 0xcccu, "a w2 write must not reach w1 at the same index");
    CHECK_EQ(read_at(tb, 0, 1), 0xbbbu, "layer_state=1 should select w2");

    // upper bits non-zero, low 9 bits = 3
    write_w2(tb, 0x603, 0x1234);          // 0b00011_000000011 -> w2 index 3
    CHECK_EQ(w2_mem[3], 0x1234u, "w2 should index on weight_addr[8:0] only");

    (void)w1_mem;
    (void)w2_mem;
    TRACE_LINE("cycles=%llu", (unsigned long long)tb.cycle());
    return tb_report();
}
