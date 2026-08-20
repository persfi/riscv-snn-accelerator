// Test part 1: first three cycles from states clear to prime0 to drain0
// Test part 2: checks path between lif_unit output and spk1 input that is managed by the sequencer

#include "Vsequencer.h"
#include "tb_harness.h"

#include <cstdint>
#include <vector>

static const int ACC_IDLE      = 0;
static const int ACC_WRITE     = 1;
static const int ACC_CLEAR     = 2;
static const int ACC_CLEAR_ALL = 3;

static const int V_IDLE      = 0;
static const int V_WRITE     = 1;
static const int V_CLEAR_ALL = 2;

static const int T_STEPS = 20;
static const int HIDDEN  = 128;
static const int N_IMAGES = 10;
static const int LANES   = 4;

// Short event lists keep DRAIN0 to 64 cycles. 
static const int EV_LEN = 2;

static std::vector<uint32_t> load_spk1() {
    std::vector<uint32_t> spk1(N_IMAGES * T_STEPS * HIDDEN, 0);
    load_hex(spk1, "verif/vectors/snn_h128_k2_T20/spk1.hex", spk1.size());
    return spk1;
}

static const uint32_t* frame(const std::vector<uint32_t>& spk1, int img, int t) {
    return &spk1[((img * T_STEPS) + t) * HIDDEN];
}

// The four lanes of one group, packed the way lif_unit inputs them.
static uint32_t group_spikes(const uint32_t* f, int group) {
    uint32_t bits = 0;
    for (int i = 0; i < LANES; i++)
        if (f[group * LANES + i]) bits |= (1u << i);
    return bits;
}

static void reset(Testbench<Vsequencer>& tb) {
    tb.top.rst = 1;
    tb.top.spike = 0;
    tb.top.start = 0;
    tb.top.bank_ready = 1;
    tb.tick();
    tb.top.rst = 0;
    tb.top.start = 1;  
    tb.tick();
    tb.top.start = 0;
    tb.settle();
}

static std::vector<std::pair<int, int>> collect_pushes(Testbench<Vsequencer>& tb,
                                                       const uint32_t* f) {
    std::vector<std::pair<int, int>> writes;
    auto& dut = tb.top;

    for (int guard = 0; guard < 4000; guard++) {
        dut.spike = group_spikes(f, dut.word_cnt_q); //group to the spikes format the sequencer will recieve
        tb.settle();

        TRACE_LINE("c=%4d ls=%d acc=%d v=%d wc=%2d wcq=%2d ev=%2d spike=%x we=%d",
                   guard, dut.layer_state, dut.acc_ctrl, dut.v_ctrl,
                   dut.word_cnt, dut.word_cnt_q, dut.ev_idx,
                   dut.spike, dut.spk1_we);

        if (dut.spk1_we) writes.push_back({dut.spk1_addr, dut.spk1_wr_data});
        else if (dut.layer_state && !writes.empty()) break;

        tb.tick();
    }
    return writes;
}

int main() {
    Testbench<Vsequencer> tb("verif/build/sequencer/sequencer.vcd");
    auto& dut = tb.top;

    dut.t_max   = T_STEPS;
    dut.eva_len = EV_LEN;
    dut.evb_len = EV_LEN;

    // --- part 1: image start ------------------------------------------------
    reset(tb);

    // CLEAR: one cycle, wipes every v and acc group before anything accumulates.
    CHECK_EQ(dut.acc_ctrl,    ACC_CLEAR_ALL, "clear: whole acc is zeroed");
    CHECK_EQ(dut.v_ctrl,      V_CLEAR_ALL,   "clear: whole v is zeroed");
    CHECK_EQ(dut.layer_state, 0,             "clear: layer 1");
    CHECK_EQ(dut.word_cnt,    0,             "clear: counter starts at 0");

    // PRIME0: no weight has arrived yet
    tb.tick();
    CHECK_EQ(dut.acc_ctrl,    ACC_IDLE, "prime0: nothing to accumulate yet");
    CHECK_EQ(dut.v_ctrl,      V_IDLE,   "prime0: v untouched");
    CHECK_EQ(dut.word_cnt,    0,        "prime0: clr held c_word at group 0");
    CHECK_EQ(dut.layer_state, 0,        "prime0: addresses layer 1");

    // DRAIN0 
    tb.tick();
    CHECK_EQ(dut.acc_ctrl,   ACC_WRITE, "drain0: group 0's weight accumulates");
    CHECK_EQ(dut.v_ctrl,     V_IDLE,    "drain0: v untouched during drain");
    CHECK_EQ(dut.word_cnt_q, 0,         "drain0: first arriving weight is group 0's");
    CHECK_EQ(dut.word_cnt,   1,         "drain0: address bus is one group ahead");
    CHECK_EQ(dut.ev_idx,     0,         "drain0: still on event 0");

    // --- part 2: spk1 push path, against golden fc1 spikes ------------------
    const std::vector<uint32_t> spk1 = load_spk1();

    for (int t = 0; t < T_STEPS; t++) {
        const uint32_t* f = frame(spk1, 0, t); //pick one img and t

        //queue of neuron idx of neurons that that spiked
        std::vector<int> expect;
        for (int n = 0; n < HIDDEN; n++)
            if (f[n]) expect.push_back(n);

        reset(tb); //rst sequencer
        std::vector<std::pair<int, int>> writes = collect_pushes(tb, f); //result from sequencer
        //pair of queue idx and spike idx of neuron

        char msg[128];
        std::snprintf(msg, sizeof msg, "t=%d: queue length matches golden spike count", t);
        CHECK_EQ((int)writes.size(), (int)expect.size(), msg);

        for (size_t i = 0; i < writes.size() && i < expect.size(); i++) {
            std::snprintf(msg, sizeof msg, "t=%d push %zu: writes to the next queue slot", t, i);
            CHECK_EQ(writes[i].first, (int)i, msg);
            std::snprintf(msg, sizeof msg, "t=%d push %zu: queues the firing neuron index", t, i);
            CHECK_EQ(writes[i].second, expect[i], msg);
        }

        TRACE_LINE("t=%2d spikes=%3zu pushes=%3zu", t, expect.size(), writes.size());
    }

    return tb_report();
}
