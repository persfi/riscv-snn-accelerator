// Control timing tests for layer 1 drain and sweep, bacuse the sequencer can not drive data. Cycle numbers are counted from the first cycle after reset.

// Assume ev_len = 2 and the sequence is: prime at 0, drain 1..64, sweep 65..96, then the next drain starts at 97.

#include "Vsequencer.h"
#include "tb_harness.h"

static const int ACC_IDLE  = 0;
static const int ACC_WRITE = 1;
static const int ACC_CLEAR = 2;
static const int ACC_CLEAR_ALL = 3;

static const int V_IDLE  = 0;
static const int V_WRITE = 1;
static const int V_CLEAR_ALL = 2;

int main() {
    Testbench<Vsequencer> tb("verif/build/sequencer/sequencer.vcd");
    auto& dut = tb.top;

    dut.t_max  = 20;
    dut.ev_len = 2;

    dut.rst = 1;
    tb.tick();
    dut.rst = 0;
    tb.settle();

    // cycle 0: prime. 
    CHECK_EQ(dut.acc_ctrl,    ACC_CLEAR_ALL, "prime: no weight has arrived yet so acc should be cleared");
    CHECK_EQ(dut.word_cnt,    0,        "prime: issues group 0's address");
    CHECK_EQ(dut.v_ctrl,      V_CLEAR_ALL,   "prime: drain v untouched, v should be cleared");
    CHECK_EQ(dut.layer_state, 0,        "prime: should address layer 1");

    // drain: word_cnt_q names the group whose weights have just arrived
    for (int c = 1; c <= 31; c++) {
        tb.tick();
        if (c == 1 || c == 15 || c == 31) {
            CHECK_EQ(dut.word_cnt,   c,         "drain: word_cnt counts normally");
            CHECK_EQ(dut.word_cnt_q, c - 1,     "drain: word_cnt_q trails word _cnt by one");
            CHECK_EQ(dut.acc_ctrl,   ACC_WRITE, "drain: the arriving weight should be accumulated");
        }
    }

    // cycle 32: event 0's last word is writing while event 1's first address is going out 
    tb.tick();
    CHECK_EQ(dut.word_cnt_q, 31,        "last word: acc writes group 31");
    CHECK_EQ(dut.ev_idx_q,   0,         "last word: the arriving weight is still event 0's");
    CHECK_EQ(dut.ev_idx,     1,         "last word: the address bus has moved to event 1 to get the right weight");
    CHECK_EQ(dut.acc_ctrl,   ACC_WRITE, "last word: still accumulating");

    // cycle 64: last word of the last event.
    for (int c = 33; c <= 64; c++) tb.tick();
    CHECK_EQ(dut.word_cnt_q, 31,        "drain end: acc writes group 31");
    CHECK_EQ(dut.ev_idx_q,   1,         "drain end: the arriving weight is event 1's");
    CHECK_EQ(dut.ev_idx,     0,         "drain end: counter is one cycle ahead ev_idx_q");
    CHECK_EQ(dut.acc_ctrl,   ACC_WRITE, "drain end: the last weight gets written");

    // cycle 65: sweep starts at group 0 with no gap
    tb.tick();
    CHECK_EQ(dut.acc_ctrl,   ACC_CLEAR, "sweep: clear the group");
    CHECK_EQ(dut.v_ctrl,     V_WRITE,   "sweep: v writes to group 0 next cycle");
    CHECK_EQ(dut.word_cnt_q, 0,         "sweep: starts at group 0");
    CHECK_EQ(dut.ev_idx_q,   0,         "sweep: event counter stalls at 0 until next layer's drain");

    // Cycle 96 is the last group and it primes the next drain, so no separate prime cycle is needed per timestep.
    for (int c = 66; c <= 96; c++) tb.tick();

    // cycle 97:  back to the drain , timestep++
    tb.tick();
    CHECK_EQ(dut.acc_ctrl,   ACC_WRITE, "loop back: draining again");
    CHECK_EQ(dut.v_ctrl,     V_IDLE,    "loop back: drain v untouched");
    CHECK_EQ(dut.word_cnt_q, 0,         "loop back: group 0's weight has arrived");
    CHECK_EQ(dut.ev_idx_q,   0,         "loop back: starts from event 0 again");
    CHECK_EQ(dut.rd_bank,    1,         "loop back: t advanced, so the other event bank is read");

    TRACE_LINE("cycles=%llu", (unsigned long long)tb.cycle());
    return tb_report();
}
