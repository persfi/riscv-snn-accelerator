
#include "Vv_mem.h"
#include "Vv_mem___024root.h"
#include "tb_harness.h"

static const int CTRL_IDLE  = 0;   // V_IDLE
static const int CTRL_WRITE = 1;   // V_WRITE
static const int CTRL_CLEAR = 2;   // V_CLEAR

static const uint64_t L1_DATA = 0x12345678aaaabcddULL;
static const uint64_t L2_DATA = 0x1111222233334444ULL;

static void write_group(Testbench<Vv_mem>& tb, int layer, int group, uint64_t data) { //group = word_cnt
    tb.top.layer_state = layer;
    tb.top.word_cnt    = group;
    tb.top.v_wdata     = data;
    tb.top.ctrl        = CTRL_WRITE;
    tb.tick();
    tb.top.ctrl = CTRL_IDLE;
}

// combinational: address and layer in, data out, no tick
static uint64_t read_group(Testbench<Vv_mem>& tb, int layer, int group) {
    tb.top.layer_state = layer;
    tb.top.word_cnt    = group;
    tb.settle();
    return tb.top.v_rdata;
}

int main() {
    Testbench<Vv_mem> tb("verif/build/v_mem/v_mem.vcd");
    auto& dut = tb.top;
    auto& v   = tb.top.rootp->v_mem__DOT__v;   // 140 x 16 bits

    dut.ctrl        = CTRL_IDLE;
    dut.layer_state = 0;
    dut.word_cnt    = 0;
    dut.v_wdata     = 0;
    tb.settle();

    // same word_cnt, different layer_state 
    write_group(tb, 0, 2, L1_DATA);
    write_group(tb, 1, 2, L2_DATA);
    CHECK_EQ(read_group(tb, 0, 2), L1_DATA, "layer 1 group 2 should hold its own data");
    CHECK_EQ(read_group(tb, 1, 2), L2_DATA, "layer 2 group 2 should hold its own data");
    CHECK_EQ(v[8],   0xbcdd, "layer 1 group 2 lane 0 writes to index 8");
    CHECK_EQ(v[136], 0x4444, "layer 2 group 2 lane 0 writes to index 128+8");

    // V_CLEAR zeroes both layers regardless of layer_state value
    dut.layer_state = 0;
    dut.ctrl= CTRL_CLEAR;
    tb.tick();
    CHECK_EQ(read_group(tb, 0, 2), 0ULL, " clear should zero layer 1");
    CHECK_EQ(read_group(tb, 1, 2), 0ULL, "the same clear should zero layer 2");

    // four distinct lane values & write timing
    dut.layer_state = 0;
    dut.word_cnt    = 5;
    dut.v_wdata     = 0x1111222233334444ULL;
    dut.ctrl        = CTRL_WRITE;
    tb.settle();
    CHECK_EQ(v[20], 0x0000, "write should not land before the posedge");
    tb.tick();
    dut.ctrl = CTRL_IDLE;

    CHECK_EQ(v[20], 0x4444, "lane 0 should land in its own entry");
    CHECK_EQ(v[21], 0x3333, "lane 1 should land in its own entry");
    CHECK_EQ(v[22], 0x2222, "lane 2 should land in its own entry");
    CHECK_EQ(v[23], 0x1111, "lane 3 should land in its own entry");
    CHECK_EQ(v[24], 0x0000, "write must not passed the assigned group");

    // combinational read
    dut.word_cnt = 2;
    tb.settle();
    CHECK_EQ(dut.v_rdata, 0ULL, "read should update without a tick");
    dut.word_cnt = 5;
    tb.settle();
    CHECK_EQ(dut.v_rdata, 0x1111222233334444ULL, "and back again, still no tick");

    // idle doesn't change memory
    dut.v_wdata = 0xDEADBEEFDEADBEEFULL;
    dut.ctrl    = CTRL_IDLE;
    tb.tick();
    CHECK_EQ(read_group(tb, 0, 5), 0x1111222233334444ULL, "ctrl=idle should  not write");

    TRACE_LINE("cycles=%llu", (unsigned long long)tb.cycle());
    return tb_report();
}
