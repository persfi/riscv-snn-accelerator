#include "Vdrain.h"
#include "tb_harness.h"

struct Case {
    int8_t      w;
    int32_t     acc_in;
    int32_t     acc_out;
    const char* what;
};

// The four rows run in lanes 0..3 at once with different weights and different
// accumulators, so a crossed lane fails as well as a wrong sum.
static const Case cases[] = {
    {    5, (int32_t)0xaaaaaaaa, (int32_t)0xaaaaaaaf, "acc adds"},
    {   -5, (int32_t)0xaaaaaaa9, (int32_t)0xaaaaaaa4, "negative weight extends correctly and carry out logic works"},
    {  127,           0x12345678,          0x123456f7, "largest positive int8"},
    { -127,                    0,                -127, "largest negative int8"},
};
static const int N_CASES = sizeof(cases) / sizeof(cases[0]);

static void set_lane(Vdrain& dut, int i, int8_t w, int32_t acc) {
    dut.weight_rdata &= ~((uint32_t)0xFF << (i * 8)); //clear lane i's byte
    dut.weight_rdata |= (uint32_t)(uint8_t)w << (i * 8);
    dut.acc_rdata[i] = (uint32_t)acc; 
}

static int32_t get_lane(Vdrain& dut, int i) {
    return (int32_t)dut.acc_wdata[i];
}

int main() {
    Testbench<Vdrain> tb("verif/build/drain/drain.vcd");
    auto& dut = tb.top;

    dut.weight_rdata = 0;
    for (int i = 0; i < 4; i++) dut.acc_rdata[i] = 0;

    // four cases per pass, one per lane, so a cross-lane wiring bug shows up alongside an arithmetic one
    for (int base = 0; base < N_CASES; base += 4) {
        int n = (N_CASES - base < 4) ? N_CASES - base : 4;
        for (int i = 0; i < n; i++) {
            const Case& c = cases[base + i];
            set_lane(dut, i, c.w, c.acc_in);
        }
        tb.settle();

        for (int i = 0; i < n; i++) {
            const Case& c = cases[base + i];
            TRACE_LINE("lane%d w=%d acc_in=%d -> acc_out=%d",
                       i, c.w, c.acc_in, get_lane(dut, i));
            CHECK_EQ(get_lane(dut, i), c.acc_out, c.what);
        }
    }

    return tb_report();
}
