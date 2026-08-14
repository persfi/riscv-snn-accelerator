#include "Vcount.h"
#include "Vcount___024root.h"
#include "tb_harness.h"

static uint8_t mem(Testbench<Vcount>& tb, int i) {
    return (uint8_t)tb.top.rootp->count__DOT__mem[i];
}

static void step(Testbench<Vcount>& tb, int en, int clr, int group, int spike) {
    tb.top.count_en  = en;
    tb.top.count_clr = clr;
    tb.top.word_cnt_q = group;
    tb.top.spike     = spike;
    tb.tick();
}

int main() {
    Testbench<Vcount> tb("verif/build/count/count.vcd");
    auto& dut = tb.top;

    step(tb, 0, 1, 0, 0);

    step(tb, 1, 0, 2, 0b1000);
    CHECK_EQ((int)mem(tb, 11), 0, "neuron 11 is padding and must not count");

    for (int i = 0; i < 5; i++) step(tb, 1, 0, 1, 0b0010);
    CHECK_EQ((int)mem(tb, 5), 5, "neuron 5 counts once per enabled spike");

    for (int i = 0; i < 5; i++) step(tb, 1, 0, 0, 0b0001);
    CHECK_EQ((int)mem(tb, 0), 5, "neuron 0 counts once per enabled spike");

    step(tb, 0, 0, 0, 0);
    dut.rd_idx = 1;
    tb.settle();
    CHECK_EQ((uint32_t)dut.rd_data, 0x00000500u, "word 1 returns neurons 4-7 with lane 0 in the low byte");

    dut.rd_idx = 0;
    tb.settle();
    CHECK_EQ((uint32_t)dut.rd_data, 0x00000005u, "word 0 returns neurons 0-3, not neurons 4-7");

    step(tb, 0, 1, 0, 0);
    for (int i = 0; i < 12; i++)
        CHECK_EQ((int)mem(tb, i), 0, "clear zeroes every counter");

    step(tb, 1, 0, 0, 0b1111);
    for (int i = 0; i < 4; i++)
        CHECK_EQ((int)mem(tb, i), 1, "all four lanes are able to count in the same cycle");

    step(tb, 0, 0, 0, 0b0001);
    CHECK_EQ((int)mem(tb, 0), 1, "a spike with count_en low must not count");

    return tb_report();
}
