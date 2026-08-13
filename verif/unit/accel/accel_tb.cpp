// First integration test: one image, one timestep, layer 1 only, checked against the golden model's vectors.

// Weights and events are loaded through the host bus, the same path the C driver will use. acc_mem and v_mem  are zeroed through rootp because the sequencer has no CLEAR state yet

// Timing for ev_len = N events, 32 words per source:
//   cycle 0            prime, first address goes out
//   cycles 1..32N      drain
//   cycle 32N+1        first sweep cycle; acc is complete and not yet cleared
//   cycles 32N+1..+32  sweep
#include "Vaccel.h"
#include "Vaccel___024root.h"
#include "tb_harness.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static const char* VEC = "verif/vectors/snn_h128_k2_T20/";

static const int IMAGE  = 0;
static const int TSTEP  = 0;
static const int T      = 20;
static const int HIDDEN = 128;
static const int WORDS  = HIDDEN / 4;   // 32 words per source

static const uint32_t W1_BASE  = 0x20020000u;
static const uint32_t EVA_BASE = 0x20001000u;

static const int V_TH = 248;
static const int K    = 2;

static std::vector<uint32_t> read_hex(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "cannot open %s (run `make vectors`)\n", path.c_str());
        std::exit(1);
    }
    std::vector<uint32_t> out;
    std::string line;
    while (std::getline(in, line)) {
        auto c = line.find("//");
        if (c != std::string::npos) line.resize(c);
        std::istringstream iss(line);
        std::string tok;
        while (iss >> tok) out.push_back((uint32_t)std::strtoul(tok.c_str(), nullptr, 16));
    }
    return out;
}

static void bus_write(Testbench<Vaccel>& tb, uint32_t addr, uint32_t data) {
    tb.top.host_addr  = addr;
    tb.top.host_wdata = data;
    tb.top.host_we    = 1;
    tb.tick();
    tb.top.host_we    = 0;
}

int main() {
    auto w1     = read_hex(std::string(VEC) + "w1.hex");
    auto ev_idx = read_hex(std::string(VEC) + "ev_idx.hex");
    auto ev_len = read_hex(std::string(VEC) + "ev_len.hex");
    auto acc1   = read_hex(std::string(VEC) + "acc1.hex");
    auto v1     = read_hex(std::string(VEC) + "v1.hex");

    const int step = IMAGE * T + TSTEP;
    const int n_ev = (int)ev_len[step];

    size_t ev_off = 0;
    for (int i = 0; i < step; i++) ev_off += ev_len[i];

    const int acc_off = step * HIDDEN;

    Testbench<Vaccel> tb("verif/build/accel/accel.vcd");
    auto& dut = tb.top;

    dut.rst       = 1;
    dut.host_we   = 0;
    dut.host_addr = 0;
    dut.t_max     = 1;
    dut.eva_len   = (uint16_t)n_ev;
    dut.v_th      = V_TH;
    dut.k         = K;
    tb.settle();

    // --- preload, with the sequencer held in reset -------------------------
    for (size_t i = 0; i < w1.size(); i++)
        bus_write(tb, W1_BASE + 4u * (uint32_t)i, w1[i]);

    for (int n = 0; n < n_ev; n++)
        bus_write(tb, EVA_BASE + 4u * (uint32_t)n, ev_idx[ev_off + n]);

    dut.rst = 0;
    tb.settle();

    // --- drain -------------------------------------------------------------
    const int drain_cycles = WORDS * n_ev;
    for (int c = 1; c <= drain_cycles + 1; c++) tb.tick();

    // acc is complete this cycle and the sweep hasn't cleared group 0 yet
    int acc_bad = 0;
    for (int i = 0; i < HIDDEN; i++) {
        int32_t got  = (int32_t)dut.rootp->accel__DOT__acc_mem__DOT__mem[i];
        int32_t want = (int32_t)acc1[acc_off + i];
        if (got != want && acc_bad++ < 8)
            std::fprintf(stderr, "  acc1[%3d]: got %d, want %d\n", i, got, want);
        CHECK_EQ(got, want, "fc1 accumulator must match the golden model");
    }
    TRACE_LINE("drain done at cycle %llu, %d events, %d mismatches",
               (unsigned long long)tb.cycle(), n_ev, acc_bad);

    // --- sweep -------------------------------------------------------------
    for (int c = 0; c < WORDS; c++) tb.tick();

    int v_bad = 0;
    for (int i = 0; i < HIDDEN; i++) {
        int16_t got  = (int16_t)dut.rootp->accel__DOT__v_mem__DOT__v[i];
        int16_t want = (int16_t)v1[acc_off + i];
        if (got != want && v_bad++ < 8)
            std::fprintf(stderr, "  v1[%3d]: got %d, want %d\n", i, got, want);
        CHECK_EQ(got, want, "fc1 membrane must match the golden model");
    }
    TRACE_LINE("sweep done at cycle %llu, %d mismatches",
               (unsigned long long)tb.cycle(), v_bad);

    return tb_report();
}
