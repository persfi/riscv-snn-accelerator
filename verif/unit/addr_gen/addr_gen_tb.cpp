// addr_gen is combinational: it turns a source index plus a word counter into a weight memory address. 
#include "Vaddr_gen.h"
#include "tb_harness.h"

struct Case {
    int         layer;
    uint8_t     shift;
    uint16_t    ev;
    uint8_t     spk1;
    uint8_t     cnt;
    uint16_t    addr;
    const char* what;
};

static const Case cases[] = {
    {0, 5, 783, 127, 31, 25087, "layer 1 max: 783*32+31, boundary of w1"},
    {0, 5,   5, 127, 12,   172, "layer 1 bit placement: 5*32+12"},
    {1, 5, 783, 127, 31,   511, "layer 2 max: 127*4+3, cnt[4:2] must be dropped"},
    {1, 5, 2,   2,  1,     9, "layer 2 bit placement: 2*4+1. Check layer select logic is correct or the output would be different."},
};
static const int N_CASES = sizeof(cases) / sizeof(cases[0]);

int main() {
    Testbench<Vaddr_gen> tb("verif/build/addr_gen/addr_gen.vcd");
    auto& dut = tb.top;

    for (int i = 0; i < N_CASES; i++) {
        const Case& c = cases[i];
        dut.layer_state  = c.layer;
        dut.shift        = c.shift;
        dut.ev_rd_data   = c.ev;
        dut.spk1_rd_data = c.spk1;
        dut.word_cnt     = c.cnt;
        tb.settle();

        TRACE_LINE("layer=%d shift=%u ev=%u spk1=%u cnt=%u -> addr=%u",
                   c.layer, c.shift, c.ev, c.spk1, c.cnt, dut.weight_addr);
        CHECK_EQ(dut.weight_addr, c.addr, c.what);
    }

    return tb_report();
}
