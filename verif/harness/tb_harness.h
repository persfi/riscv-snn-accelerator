// Reusable driver for per-block Verilator unit tests: owns the DUT, a VCD
// trace, and clock stepping. Stimulus and assertions belong in each *_tb.cpp,
// not here.
#pragma once

#include <verilated.h>
#include <verilated_vcd_c.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <type_traits>

// Detects whether TOP has a clk port, so Testbench works for both clocked
// and purely combinational DUTs (e.g. the ALU) without a separate class.
namespace tb_detail {
template <typename T, typename = void>
struct has_clk : std::false_type {};
template <typename T>
struct has_clk<T, std::void_t<decltype(std::declval<T&>().clk)>>
    : std::true_type {};
}  // namespace tb_detail

template <typename TOP>
class Testbench {
public:
    // Declaration order is construction order: ctx_ must exist before top
    // is built with a pointer to it, regardless of initializer-list order.
    VerilatedContext ctx_;
    TOP top;

    explicit Testbench(const char* trace_path) : top{&ctx_} {
        ctx_.traceEverOn(true);
        top.trace(&trace_, 99);
        trace_.open(trace_path);
        if constexpr (tb_detail::has_clk<TOP>::value) {
            top.clk = 0;
        }
        settle();
    }

    ~Testbench() {
        trace_.close();
        top.final();
    }

    // Re-evaluates combinational logic without moving the clock.
    void settle() {
        top.eval();
        trace_.dump(time_);
        ++time_;
    }

    void negedge() { top.clk = 0; settle(); }
    void posedge() { top.clk = 1; settle(); }
    void tick()    { negedge(); posedge(); ++cycles_; }

    // Completed tick() count -- for trace lines and cycle-bounded
    // integration loops ("run N cycles, then check state"). Distinct from
    // time_, which is a VCD timestamp that also moves on settle()/negedge().
    vluint64_t cycle() const { return cycles_; }

private:
    VerilatedVcdC trace_;
    vluint64_t time_ = 0;
    vluint64_t cycles_ = 0;
};

// --- minimal self-checking assertions -------------------------------------

inline int& tb_checks()   { static int n = 0; return n; }
inline int& tb_failures() { static int n = 0; return n; }

#define CHECK_EQ(actual, expected, msg)                                     \
    do {                                                                    \
        ++tb_checks();                                                      \
        auto tb_actual_ = (actual);                                         \
        auto tb_expected_ = (expected);                                     \
        if (!(tb_actual_ == tb_expected_)) {                                \
            ++tb_failures();                                                \
            std::fprintf(stderr, "FAIL %s:%d: %s (got 0x%lx, want 0x%lx)\n", \
                          __FILE__, __LINE__, msg,                          \
                          (unsigned long)tb_actual_,                        \
                          (unsigned long)tb_expected_);                     \
        }                                                                   \
    } while (0)

inline int tb_report() {
    std::printf("%d/%d checks passed\n", tb_checks() - tb_failures(), tb_checks());
    return tb_failures() ? 1 : 0;
}

// CHECK_EQ wrappers against a peeked array element, from `public_flat_rd`/`_rw` regfile or memory exposed through rootp. 
#define CHECK_REG(n, val, msg) CHECK_EQ(regs[n], (uint32_t)(val), msg)
#define CHECK_MEM(n, val, msg) CHECK_EQ(dmem_arr[n], (uint32_t)(val), msg)

// optional per-cycle trace line 
// Prints a formatted, printf-style line only when the TRACE env var is set, so integration tests can show cycle-by-cycle state (e.g. "cycle=%d pc=%08x inst=%08x") without cluttering default/CI runs. Field list is upto each *_tb.cpp: grows (writeback reg/value, etc.) without needing any changes here.

inline bool tb_trace_enabled() {
    static const bool enabled = std::getenv("TRACE") != nullptr;
    return enabled;
}

#define TRACE_LINE(...)                  \
    do {                                  \
        if (tb_trace_enabled()) {         \
            std::printf(__VA_ARGS__);     \
            std::printf("\n");            \
        }                                 \
    } while (0) //to prevent mismatching the if statement to a different else

template <typename Mem>
void load_hex(Mem& mem, const char* path, size_t depth) {
    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "load_hex: cannot open %s\n", path);
        std::exit(1);
    }
    size_t index = 0, skipped = 0;
    std::string line;
    while (std::getline(in, line)) {
        auto comment = line.find("//");
        if (comment != std::string::npos) line.resize(comment);
        std::istringstream iss(line);
        std::string tok;
        while (iss >> tok) {
            if (tok[0] == '@') {
                index = std::strtoul(tok.c_str() + 1, nullptr, 16);
                continue;
            }
            // Not fatal: imem only fetches .text at the low addresses, so a program whose
            // .rodata runs past DEPTH still boots. Report the count anyway, since a .text
            // that genuinely overflowed looks identical from here.
            if (index >= depth) { skipped++; continue; }
            mem[index++] = static_cast<uint32_t>(std::strtoul(tok.c_str(), nullptr, 16));
        }
    }
    if (skipped)
        std::fprintf(stderr, "load_hex: %s truncated to depth=%zu (%zu words dropped)\n",
                     path, depth, skipped);
}

// Load a hex into mem starting at a given word offset
template <typename Mem>
void load_hex_at(Mem& mem, const char* path, size_t word_offset, size_t depth) {
    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "load_hex_at: cannot open %s\n", path);
        std::exit(1);
    }
    size_t index = word_offset;
    std::string line;
    while (std::getline(in, line)) {
        auto comment = line.find("//");
        if (comment != std::string::npos) line.resize(comment);
        std::istringstream iss(line);
        std::string tok;
        while (iss >> tok) {
            if (index >= depth) {
                std::fprintf(stderr,
                             "load_hex_at: %s does not fit at word %zu in depth=%zu\n",
                             path, word_offset, depth);
                std::exit(1);
            }
            mem[index++] = static_cast<uint32_t>(std::strtoul(tok.c_str(), nullptr, 16));
        }
    }
}
