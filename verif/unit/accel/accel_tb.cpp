// Check layer 1 against the golden model, two timesteps. Weights and events go in
// through the host bus, the same path the C driver will use.
//
// cycle 0 is the prime; then each timestep is 32*ev_len drain cycles followed
// by 32 sweep cycles. Bank A holds even timesteps, bank B odd.

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
static const int STEPS  = 2;
static const int T      = 20;
static const int HIDDEN = 128;
static const int WORDS  = HIDDEN / 4;

static const uint32_t W1_BASE  = 0x20020000u;
static const uint32_t EVA_BASE = 0x20001000u;
static const uint32_t EVB_BASE = 0x20002000u;

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

static void cmp_acc(Testbench<Vaccel>& tb, const std::vector<uint32_t>& gold, int off, int t) {
    int bad = 0;
    for (int i = 0; i < HIDDEN; i++) {
        int32_t got  = (int32_t)tb.top.rootp->accel__DOT__acc_mem__DOT__mem[i];
        int32_t want = (int32_t)gold[off + i];
        if (got != want && bad++ < 4)
            std::fprintf(stderr, "  t%d acc1[%3d]: got %d, want %d\n", t, i, got, want);
        CHECK_EQ(got, want, "fc1 accumulator must match the golden model");
    }
}

static void cmp_v(Testbench<Vaccel>& tb, const std::vector<uint32_t>& gold, int off, int t) {
    int bad = 0;
    for (int i = 0; i < HIDDEN; i++) {
        int16_t got  = (int16_t)tb.top.rootp->accel__DOT__v_mem__DOT__v[i];
        int16_t want = (int16_t)gold[off + i];
        if (got != want && bad++ < 4)
            std::fprintf(stderr, "  t%d v1[%3d]: got %d, want %d\n", t, i, got, want);
        CHECK_EQ(got, want, "fc1 membrane must match the golden model");
    }
}

int main() {
    auto w1     = read_hex(std::string(VEC) + "w1.hex");
    auto ev_idx = read_hex(std::string(VEC) + "ev_idx.hex");
    auto ev_len = read_hex(std::string(VEC) + "ev_len.hex");
    auto acc1   = read_hex(std::string(VEC) + "acc1.hex");
    auto v1     = read_hex(std::string(VEC) + "v1.hex");

    const int step0 = IMAGE * T + TSTEP;

    size_t ev_off[STEPS]; //where every image's idx starts in ev_idx hex
    ev_off[0] = 0;
    for (int i = 0; i < step0; i++) ev_off[0] += ev_len[i]; //ev_len from each timestep
    for (int s = 1; s < STEPS; s++) ev_off[s] = ev_off[s - 1] + ev_len[step0 + s - 1];

    Testbench<Vaccel> tb("verif/build/accel/accel.vcd");
    auto& dut = tb.top;

    dut.rst       = 1;
    dut.host_we   = 0;
    dut.host_addr = 0;
    dut.t_max     = STEPS;
    dut.eva_len   = (uint16_t)ev_len[step0];
    dut.evb_len   = (uint16_t)ev_len[step0 + 1];
    dut.v_th      = V_TH;
    dut.k         = K;
    tb.settle();

    for (size_t i = 0; i < w1.size(); i++)
        bus_write(tb, W1_BASE + 4u * (uint32_t)i, w1[i]);

    for (int s = 0; s < STEPS; s++) {
        uint32_t base = (s & 1) ? EVB_BASE : EVA_BASE;
        for (uint32_t n = 0; n < ev_len[step0 + s]; n++)
            bus_write(tb, base + 4u * n, ev_idx[ev_off[s] + n]); //writes to the correct images array idx
    }

    dut.rst = 0;
    tb.settle();

    for (int s = 0; s < STEPS; s++) {
        const int step = step0 + s;
        const int off  = step * HIDDEN;

        // +1 on the first timestep for the prime; later ones are primed by the last cycle of the previous timestep
        int ticks = WORDS * (int)ev_len[step] + (s == 0 ? 1 : 0);
        for (int c = 0; c < ticks; c++) tb.tick();
        cmp_acc(tb, acc1, off, step);
        TRACE_LINE("t%d drain done at cycle %llu, %d events",
                   step, (unsigned long long)tb.cycle(), (int)ev_len[step]);

        for (int c = 0; c < WORDS; c++) tb.tick();
        cmp_v(tb, v1, off, step);
        TRACE_LINE("t%d sweep done at cycle %llu", step, (unsigned long long)tb.cycle());
    }

    return tb_report();
}
