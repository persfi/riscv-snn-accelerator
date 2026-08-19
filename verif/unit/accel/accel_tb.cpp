// Two full images, 20 timesteps each, against the golden model at every state
// boundary, plus the host read path for status and the output counts.

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "Vaccel.h"
#include "Vaccel___024root.h"
#include "tb_harness.h"

static const char* VEC = "verif/vectors/snn_h128_k2_T20/";

static const int IMAGES = 10;
static const int T = 20;
static const int HIDDEN = 128;
static const int OUTS = 10;
static const int LANES = 4;

static const uint32_t ACCEL_BASE = 0x20000000u;
static const uint32_t W1_BASE = 0x20020000u;
static const uint32_t W2_BASE = 0x20040000u;
static const uint32_t EVA_BASE = 0x20001000u;
static const uint32_t EVB_BASE = 0x20002000u;

static const uint32_t T_ADDR = ACCEL_BASE + 0x10u;
static const uint32_t VTH1_ADDR = ACCEL_BASE + 0x14u;
static const uint32_t VTH2_ADDR = ACCEL_BASE + 0x18u;
static const uint32_t K_ADDR = ACCEL_BASE + 0x1Cu;
static const uint32_t START_ADDR = ACCEL_BASE + 0x20u;
static const uint32_t EVA_LEN_ADDR = ACCEL_BASE + 0x24u;
static const uint32_t EVB_LEN_ADDR = ACCEL_BASE + 0x28u;
static const uint32_t STATUS_ADDR = ACCEL_BASE + 0x2Cu;
static const uint32_t COUNT_ADDR = ACCEL_BASE + 0x40u;
static const uint32_t UNMAPPED_ADDR = ACCEL_BASE + 0x34u;

// mmio.h status bits
static const uint32_t BANK_A_FREE_BIT = 1u << 0;
static const uint32_t BANK_B_FREE_BIT = 1u << 1;
static const uint32_t IMAGE_DONE_BIT = 1u << 2;

static const int V_TH1 = 248;
static const int V_TH2 = 295;
static const int K = 2;

enum {
  CLEAR = 0,
  PRIME0 = 1,
  DRAIN0 = 2,
  SWEEP0 = 3,
  PRIME1 = 4,
  DRAIN1 = 5,
  SWEEP1 = 6,
  IDLE = 7
};

static std::vector<uint32_t> w1, w2, ev_idx, ev_len, acc1, v1, spk1g, acc2, v2,
    counts;
static std::vector<size_t>
    ev_start;  // where each (image,timestep) begins in ev_idx
static bool saw_image_done = false;

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
    while (iss >> tok)
      out.push_back((uint32_t)std::strtoul(tok.c_str(), nullptr, 16));
  }
  return out;
}

static void bus_write(Testbench<Vaccel>& tb, uint32_t addr, uint32_t data) {
  tb.top.host_addr = addr;
  tb.top.host_wdata = data;
  tb.top.host_we = 1;
  tb.tick();
  tb.top.host_we = 0;
  if (tb.top.image_done) saw_image_done = true;
}

// Reads are combinational off host_addr, so no clock edge passes and the
// accelerator does not advance. Safe to read during a running image.
static uint32_t bus_read(Testbench<Vaccel>& tb, uint32_t addr) {
  tb.top.host_we = 0;
  tb.top.host_addr = addr;
  tb.settle();
  return (uint32_t)tb.top.host_rdata;
}

static int state_of(Testbench<Vaccel>& tb) {
  return (int)tb.top.rootp->accel__DOT__sequencer__DOT__state;
}

static bool run_until(Testbench<Vaccel>& tb, int want, const char* name) {
  for (int i = 0; i < 200000; i++) {
    tb.tick();
    if (tb.top.image_done) saw_image_done = true;
    if (state_of(tb) == want) return true;
  }
  std::fprintf(stderr, "TIMEOUT waiting for %s (stuck in state %d)\n", name,
               state_of(tb));
  return false;
}

static void fill_bank(Testbench<Vaccel>& tb, int step, bool bank_b) {
  uint32_t base = bank_b ? EVB_BASE : EVA_BASE;
  for (uint32_t n = 0; n < ev_len[step]; n++)
    bus_write(tb, base + 4u * n, ev_idx[ev_start[step] + n]);
  bus_write(tb, bank_b ? EVB_LEN_ADDR : EVA_LEN_ADDR, ev_len[step]);
}

// One image, start to IDLE. Assumes the accelerator is sitting in IDLE and the
// boot-time config registers are already written.
static bool run_image(Testbench<Vaccel>& tb, int img) {
  auto& dut = tb.top;
  char msg[160];

  saw_image_done = false;

  // done only changes when reset or the next start clears it
  std::snprintf(msg, sizeof msg,
                "img %d: done still reflects the previous image before start",
                img);
  CHECK_EQ(bus_read(tb, STATUS_ADDR) & IMAGE_DONE_BIT,
           img == 0 ? 0u : IMAGE_DONE_BIT, msg);
  std::snprintf(msg, sizeof msg,
                "img %d: bank A reads free before it is filled", img);
  CHECK_EQ(bus_read(tb, STATUS_ADDR) & BANK_A_FREE_BIT, BANK_A_FREE_BIT, msg);

  fill_bank(tb, img * T, false);

  std::snprintf(msg, sizeof msg, "img %d: writing eva_len marks bank A taken",
                img);
  CHECK_EQ(bus_read(tb, STATUS_ADDR) & BANK_A_FREE_BIT, 0u, msg);

  bus_write(tb, START_ADDR, 1);

  std::snprintf(msg, sizeof msg, "img %d: start clears done", img);
  CHECK_EQ(bus_read(tb, STATUS_ADDR) & IMAGE_DONE_BIT, 0u, msg);

  for (int t = 0; t < T; t++) {
    const int step = img * T + t;
    const int off1 = step * HIDDEN;
    const int off2 = step * OUTS;

    if (t > 0 && !run_until(tb, DRAIN0, "DRAIN0")) return false;

    // Refill the bank this timestep is not reading and set its length.
    if (t + 1 < T) fill_bank(tb, step + 1, ((t + 1) & 1) != 0);

    if (!run_until(tb, SWEEP0, "SWEEP0")) return false;
    for (int i = 0; i < HIDDEN; i++) {
      std::snprintf(msg, sizeof msg,
                    "img %d t=%d acc1[%d] must match the golden model", img, t,
                    i);
      CHECK_EQ((int32_t)dut.rootp->accel__DOT__acc_mem__DOT__mem[i],
               (int32_t)acc1[off1 + i], msg);
    }

    if (!run_until(tb, PRIME1, "PRIME1")) return false;
    for (int i = 0; i < HIDDEN; i++) {
      std::snprintf(msg, sizeof msg,
                    "img %d t=%d v1[%d] must match the golden model", img, t,
                    i);
      CHECK_EQ((int16_t)dut.rootp->accel__DOT__v_mem__DOT__v[i],
               (int16_t)v1[off1 + i], msg);
    }

    std::vector<int> gold_q;
    for (int n = 0; n < HIDDEN; n++)
      if (spk1g[off1 + n]) gold_q.push_back(n);

    if (!run_until(tb, DRAIN1, "DRAIN1")) return false;
    std::snprintf(msg, sizeof msg,
                  "img %d t=%d: spk1 length must equal the golden spike count",
                  img, t);
    CHECK_EQ((int)dut.rootp->accel__DOT__sequencer__DOT__spk1_wr_ptr,
             (int)gold_q.size(), msg);
    std::snprintf(msg, sizeof msg,
                  "img %d t=%d: spk1 must hold the golden firing neurons", img,
                  t);
    for (size_t i = 0; i < gold_q.size(); i++)
      CHECK_EQ((int)dut.rootp->accel__DOT__spk1__DOT__mem[i], gold_q[i], msg);

    if (!run_until(tb, SWEEP1, "SWEEP1")) return false;
    std::snprintf(msg, sizeof msg,
                  "img %d t=%d: acc2 must match the golden model", img, t);
    for (int i = 0; i < OUTS; i++)
      CHECK_EQ((int32_t)dut.rootp->accel__DOT__acc_mem__DOT__mem[i],
               (int32_t)acc2[off2 + i], msg);

    // the last timestep returns to IDLE instead of prime
    if (!run_until(tb, t == T - 1 ? IDLE : PRIME0,
                   t == T - 1 ? "IDLE" : "PRIME0"))
      return false;
    std::snprintf(msg, sizeof msg,
                  "img %d t=%d: v2 must match the golden model", img, t);
    for (int i = 0; i < OUTS; i++)
      CHECK_EQ((int16_t)dut.rootp->accel__DOT__v_mem__DOT__v[128 + i],
               (int16_t)v2[off2 + i], msg);

    if (t == T / 2) {
      std::snprintf(msg, sizeof msg,
                    "img %d: done is still clear mid-image at t=%d", img, t);
      CHECK_EQ(bus_read(tb, STATUS_ADDR) & IMAGE_DONE_BIT, 0u, msg);
    }

    TRACE_LINE("img %d t=%2d done at cycle %llu, bank %d, %d events", img, t,
               (unsigned long long)tb.cycle(), t & 1, (int)ev_len[step]);
  }

  for (int i = 0; i < OUTS; i++) {
    std::snprintf(msg, sizeof msg,
                  "img %d: count[%d] must match the golden model", img, i);
    CHECK_EQ((int)dut.rootp->accel__DOT__count__DOT__mem[i],
             (int)counts[img * OUTS + i], msg);
  }

  std::snprintf(msg, sizeof msg,
                "img %d: image_done must assert on the last timestep", img);
  CHECK_EQ((int)saw_image_done, 1, msg);

  std::snprintf(msg, sizeof msg, "img %d: done is set once the image finishes",
                img);
  CHECK_EQ(bus_read(tb, STATUS_ADDR) & IMAGE_DONE_BIT, IMAGE_DONE_BIT, msg);

  for (int i = 0; i < 8; i++) {
    tb.tick();
    std::snprintf(msg, sizeof msg, "img %d: done stays set on poll %d", img, i);
    CHECK_EQ(bus_read(tb, STATUS_ADDR) & IMAGE_DONE_BIT, IMAGE_DONE_BIT, msg);
  }

  // Same golden values as the peek above, fetched through mmio bus.
  for (int w = 0; w < 3; w++) {
    uint32_t got = bus_read(tb, COUNT_ADDR + 4u * (uint32_t)w);
    for (int lane = 0; lane < LANES; lane++) {
      int n = w * LANES + lane;
      if (n >= OUTS) continue;
      std::snprintf(msg, sizeof msg,
                    "img %d read: count word %d lane %d is neuron %d's total",
                    img, w, lane, n);
      CHECK_EQ((int)((got >> (8 * lane)) & 0xFFu), (int)counts[img * OUTS + n],
               msg);
    }
  }

  std::snprintf(msg, sizeof msg,
                "img %d read: an unmapped register returns zero", img);
  CHECK_EQ(bus_read(tb, UNMAPPED_ADDR), 0u, msg);

  return true;
}

int main() {
  w1 = read_hex(std::string(VEC) + "w1.hex");
  w2 = read_hex(std::string(VEC) + "w2.hex");
  ev_idx = read_hex(std::string(VEC) + "ev_idx.hex");
  ev_len = read_hex(std::string(VEC) + "ev_len.hex");
  acc1 = read_hex(std::string(VEC) + "acc1.hex");
  v1 = read_hex(std::string(VEC) + "v1.hex");
  spk1g = read_hex(std::string(VEC) + "spk1.hex");
  acc2 = read_hex(std::string(VEC) + "acc2.hex");
  v2 = read_hex(std::string(VEC) + "v2.hex");
  counts = read_hex(std::string(VEC) + "counts.hex");

  ev_start.resize(ev_len.size());
  size_t acc = 0;
  for (size_t i = 0; i < ev_len.size(); i++) {
    ev_start[i] = acc;
    acc += ev_len[i];
  }

  Testbench<Vaccel> tb("verif/build/accel/accel.vcd");
  auto& dut = tb.top;

  dut.rst = 1;
  dut.host_we = 0;
  dut.host_addr = 0;
  tb.settle();

  /* weights arrive via $readmemh(INIT_FILE) in w_mem, not over the bus.*/
  tb.tick(); //reset clk

  dut.rst = 0;
  tb.settle();

  // boot-time config: written once
  bus_write(tb, T_ADDR, T);
  bus_write(tb, VTH1_ADDR, V_TH1);
  bus_write(tb, VTH2_ADDR, V_TH2);
  bus_write(tb, K_ADDR, K);

  for (int img = 0; img < IMAGES; img++)
    if (!run_image(tb, img)) return tb_report();

  return tb_report();
}
