// Full-path check of one timestep: reset -> DRAIN0 -> SWEEP0 -> PRIME1 ->
// DRAIN1 -> SWEEP1, against the golden model.

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
static const int OUTS   = 10;

static const uint32_t ACCEL_BASE = 0x20000000u;
static const uint32_t W1_BASE    = 0x20020000u;
static const uint32_t W2_BASE    = 0x20040000u;
static const uint32_t EVA_BASE   = 0x20001000u;

static const uint32_t T_ADDR       = ACCEL_BASE + 0x10u;
static const uint32_t VTH1_ADDR    = ACCEL_BASE + 0x14u;
static const uint32_t VTH2_ADDR    = ACCEL_BASE + 0x18u;
static const uint32_t K_ADDR       = ACCEL_BASE + 0x1Cu;
static const uint32_t EVA_LEN_ADDR = ACCEL_BASE + 0x24u;
static const uint32_t EVB_LEN_ADDR = ACCEL_BASE + 0x28u;

static const int V_TH1 = 248;
static const int V_TH2 = 295;
static const int K     = 2;

// sequencer state encoding
enum { CLEAR = 0, PRIME0 = 1, DRAIN0 = 2, SWEEP0 = 3, PRIME1 = 4, DRAIN1 = 5, SWEEP1 = 6 };

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
//write weight and event bank a
static void bus_write(Testbench<Vaccel>& tb, uint32_t addr, uint32_t data) {
    tb.top.host_addr  = addr;
    tb.top.host_wdata = data;
    tb.top.host_we    = 1;
    tb.tick();
    tb.top.host_we    = 0;
}

static int state_of(Testbench<Vaccel>& tb) {
    return (int)tb.top.rootp->accel__DOT__sequencer__DOT__state;
}

static bool run_until(Testbench<Vaccel>& tb, int want, const char* name) {
    for (int i = 0; i < 40000; i++) {
        tb.tick();
        if (state_of(tb) == want) { //check if its within the desired state
            TRACE_LINE("reached %s at cycle %llu", name, (unsigned long long)tb.cycle());
            return true;
        }
    }
    std::fprintf(stderr, "TIMEOUT waiting for %s (stuck in state %d)\n", name, state_of(tb));
    return false;
}

int main() {
    auto w1     = read_hex(std::string(VEC) + "w1.hex");
    auto w2     = read_hex(std::string(VEC) + "w2.hex");
    auto ev_idx = read_hex(std::string(VEC) + "ev_idx.hex");
    auto ev_len = read_hex(std::string(VEC) + "ev_len.hex");
    auto acc1   = read_hex(std::string(VEC) + "acc1.hex");
    auto v1     = read_hex(std::string(VEC) + "v1.hex");
    auto spk1g  = read_hex(std::string(VEC) + "spk1.hex");
    auto acc2   = read_hex(std::string(VEC) + "acc2.hex");
    auto v2     = read_hex(std::string(VEC) + "v2.hex");

    const int step = IMAGE * T + TSTEP;
    const int off1 = step * HIDDEN;
    const int off2 = step * OUTS;

    size_t ev_off = 0;
    for (int i = 0; i < step; i++) ev_off += ev_len[i];

    // golden queue: firing hidden neurons, ascending
    std::vector<int> gold_q;
    for (int n = 0; n < HIDDEN; n++)
        if (spk1g[off1 + n]) gold_q.push_back(n);

    Testbench<Vaccel> tb("verif/build/accel/accel.vcd");
    auto& dut = tb.top;

    dut.rst       = 1;
    dut.host_we   = 0;
    dut.host_addr = 0;
    tb.settle();

    // write weights and event. (w_mem and ev_mem have no rst, so they keep what is written)
    for (size_t i = 0; i < w1.size(); i++) bus_write(tb, W1_BASE + 4u * (uint32_t)i, w1[i]);
    for (size_t i = 0; i < w2.size(); i++) bus_write(tb, W2_BASE + 4u * (uint32_t)i, w2[i]);
    for (uint32_t n = 0; n < ev_len[step]; n++)
        bus_write(tb, EVA_BASE + 4u * n, ev_idx[ev_off + n]);

    dut.rst = 0;
    tb.settle();

    // config after rst
    bus_write(tb, EVA_LEN_ADDR, ev_len[step]);
    bus_write(tb, EVB_LEN_ADDR, ev_len[step]);
    bus_write(tb, T_ADDR,       T);
    bus_write(tb, VTH1_ADDR,    V_TH1);
    bus_write(tb, VTH2_ADDR,    V_TH2);
    bus_write(tb, K_ADDR,       K);

    //end of DRAIN0
    if (!run_until(tb, SWEEP0, "SWEEP0")) return tb_report();
    for (int i = 0; i < HIDDEN; i++)
        CHECK_EQ((int32_t)tb.top.rootp->accel__DOT__acc_mem__DOT__mem[i],
                 (int32_t)acc1[off1 + i], "acc1 must match the golden model");

    //end of SWEEP0
    if (!run_until(tb, PRIME1, "PRIME1")) return tb_report();
    for (int i = 0; i < HIDDEN; i++)
        CHECK_EQ((int16_t)tb.top.rootp->accel__DOT__v_mem__DOT__v[i],
                 (int16_t)v1[off1 + i], "v1 must match the golden model");

    //start of DRAIN1
    if (!run_until(tb, DRAIN1, "DRAIN1")) return tb_report();
    CHECK_EQ((int)tb.top.rootp->accel__DOT__sequencer__DOT__spk1_wr_ptr,
             (int)gold_q.size(), "spk1 length must equal the golden spike count");
    for (size_t i = 0; i < gold_q.size(); i++)
        CHECK_EQ((int)tb.top.rootp->accel__DOT__spk1__DOT__mem[i], gold_q[i],
                 "spk1 slot must hold the golden firing neuron");

    //end of DRAIN1
    
    if (!run_until(tb, SWEEP1, "SWEEP1")) return tb_report();
    for (int i = 0; i < OUTS; i++)
        CHECK_EQ((int32_t)tb.top.rootp->accel__DOT__acc_mem__DOT__mem[i],
                 (int32_t)acc2[off2 + i], "acc2 must match the golden model");

    //end of SWEEP1
    if (!run_until(tb, PRIME0, "PRIME0")) return tb_report();
    for (int i = 0; i < OUTS; i++)
        CHECK_EQ((int16_t)tb.top.rootp->accel__DOT__v_mem__DOT__v[128 + i],
                 (int16_t)v2[off2 + i], "v2 must match the golden model");

    return tb_report();
}
