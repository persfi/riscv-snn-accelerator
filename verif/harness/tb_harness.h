// Reusable driver for per-block Verilator unit tests: owns the DUT, a VCD
// trace, and clock stepping. Stimulus and assertions belong in each *_tb.cpp,
// not here.
#pragma once

#include <verilated.h>
#include <verilated_vcd_c.h>

#include <cstdio>
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
    void tick()    { negedge(); posedge(); }

private:
    VerilatedVcdC trace_;
    vluint64_t time_ = 0;
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
