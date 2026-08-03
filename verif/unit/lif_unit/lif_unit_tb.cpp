// lif_unit is combinational, so no clock. Cases are
// packed four at a time, one per lane, with different values in each lane so a
// cross-lane wiring bug shows up as well as a maths bug.
#include "Vlif_unit.h"
#include "tb_harness.h"

static const int V_TH = 248;
static const int K    = 2;

struct Case {
    int16_t v;
    int32_t acc;
    int16_t v_res;
    int     spike;
    const char* what;
};

static const Case cases[] = {
    {     1,       0,      1, 0, "dead zone: 0<v<4 does not leak at k=2"},
    {    -1,       0,      0, 0, "-1 shifts to 0: -1 - (-1)"},
    {    -5,       0,     -3, 0, "should shift to -3"},
    { 32760,    1000,  25322, 1, "leak, fire, subtract, all in range"},
    { 32760,    8445,  32767, 1, "out of range before fire, fits after: catches clamp order"},
    {   300,   99568,  32767, 1, "positive clamp"},
    {-32768, -100000, -32768, 0, "negative clamp"},
    {     0,     248,    248, 0, "exactly at threshold: strict > so no fire"},
};
static const int N_CASES = sizeof(cases) / sizeof(cases[0]);

static void set_lane(Vlif_unit& dut, int i, int16_t v, int32_t acc) {
    dut.v &= ~((uint64_t)0xFFFF << (i * 16)); //clear lane to 0x0000 while others to 0xFFFF and (&) AND it with dut.v so intended lane stays 0 while the others are their original value
    dut.v |= (uint64_t)(uint16_t)v << (i * 16); //convert to 16bit and move to intended lane
    dut.acc[i] = (uint32_t)acc;
}

static int16_t get_lane(Vlif_unit& dut, int i) {
    return (int16_t)((dut.v_res >> (i * 16)) & 0xFFFF);
}

int main() {
    Testbench<Vlif_unit> tb("verif/build/lif_unit/lif_unit.vcd");
    auto& dut = tb.top;

    dut.k    = K;
    dut.v_th = V_TH;

    // four cases per pass, one per lane
    for (int base = 0; base < N_CASES; base += 4) {
        for (int i = 0; i < 4; i++) {
            const Case& c = cases[base + i];
            set_lane(dut, i, c.v, c.acc);
        }
        tb.settle();

        for (int i = 0; i < 4; i++) {
            const Case& c = cases[base + i];
            TRACE_LINE("lane%d v=%d acc=%d -> v_res=%d spike=%d",
                       i, c.v, c.acc, get_lane(dut, i), (dut.spike >> i) & 1);
            CHECK_EQ(get_lane(dut, i), c.v_res, c.what); //check v_res
            CHECK_EQ((dut.spike >> i) & 1, c.spike, c.what); //check spikes
        }
    }

    //check different k and v_th
    dut.k    = 1;   //leak halves instead of a quarter
    dut.v_th = V_TH;
    set_lane(dut, 0, 100, 0);
    tb.settle();
    TRACE_LINE("k=1 v=100 acc=0 -> v_res=%d", get_lane(dut, 0));
    CHECK_EQ(get_lane(dut, 0), 50, "k must drive the leak shift, not a hardcoded 2");

    dut.k    = K;
    dut.v_th = 295;  //layer 2's threshold
    set_lane(dut, 0, 0, 295);
    tb.settle();
    TRACE_LINE("v_th=295 v=0 acc=295 -> v_res=%d spike=%d",
               get_lane(dut, 0), dut.spike & 1);
    CHECK_EQ(get_lane(dut, 0), 295, "v_th must drive the compare, not a hardcoded 248");
    CHECK_EQ(dut.spike & 1, 0, "at v_th=295 exactly, strict > so no fire");

    return tb_report();
}
