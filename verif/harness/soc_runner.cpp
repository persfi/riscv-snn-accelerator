#include "Vsoc.h"
#include "Vsoc___024root.h"
#include "tb_harness.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <program.hex> [max_cycles]\n", argv[0]);
        return 2;
    }
    const char* hex_path = argv[1];
    const uint64_t max_cycles =
        (argc > 2 && argv[2][0] != '-') ? std::strtoull(argv[2], nullptr, 0)
                                        : 3000000;

    // --dump-events: sw/apps/encode.c fills event bank A with the indices that fired, then writes the count of them to PRINT_INT. With this flag each count is followed by that many entries of bank A, so stdout carries both ev_len and ev_idx.
    bool dump_events = false;
    for (int i = 1; i < argc; i++)
        if (std::strcmp(argv[i], "--dump-events") == 0) dump_events = true;

    Testbench<Vsoc> tb("verif/build/soc-run/run.vcd");
    auto& dut  = tb.top;
    auto& imem = tb.top.rootp->soc__DOT__core__DOT__imem__DOT__mem;
    auto& dmem = tb.top.rootp->soc__DOT__dmem__DOT__mem;

    // Depths read straight from the Verilated arrays, so they track the RTL's
    // DEPTH params with no second source of truth (same trick as the runner).
    const size_t idepth = sizeof(imem) / sizeof(imem[0]);
    const size_t ddepth = sizeof(dmem) / sizeof(dmem[0]);
    load_hex(imem, hex_path, idepth);
    load_hex(dmem, hex_path, ddepth);

    // Core-only benchmark: the weights are far too big for the program image, so
    // the harness drops them straight into dmem at the addresses mnist_core.c
    // reads. 
#ifdef W1_HEX
    load_hex_at(dmem, W1_HEX, 0x4000 / 4, ddepth);
#endif
#ifdef W2_HEX
    load_hex_at(dmem, W2_HEX, 0x1D000 / 4, ddepth);
#endif

    // Reset one cycle, then release. The computation baseline is measured from
    // release, so snapshot the cycle count there.
    dut.rst = 1;
    tb.tick();
    dut.rst = 0;
    const uint64_t start = tb.cycle();

    // PRIME0 is the only state that waits on the host: it holds until bank_ready
    // says the next timestep's events have been written. 
    enum { SEQ_PRIME0 = 1, SEQ_IDLE = 7 };
    auto& seq_state = tb.top.rootp->soc__DOT__accel__DOT__sequencer__DOT__state;
    uint64_t busy = 0, stalled = 0;

    bool saw_int = false;
    uint32_t last_int = 0;

    enum { MARK_START = 0, MARK_ENCODE = 1, MARK_EVAL = 2 };
    uint64_t enc_cycles = 0, eval_cycles = 0, phase_start = 0;
    bool saw_mark = false;

    while (tb.cycle() < max_cycles) {
        tb.tick();
        if (seq_state != SEQ_IDLE) {
            if (seq_state == SEQ_PRIME0) stalled++;
            else busy++;
        }
        if (dut.print_sel) {
            const uint8_t c = (uint8_t)dut.print_data;
            if (c <= MARK_EVAL) {
                const uint64_t now = tb.cycle();
                if (c == MARK_ENCODE) enc_cycles += now - phase_start;
                else if (c == MARK_EVAL) eval_cycles += now - phase_start;
                phase_start = now;
                saw_mark = true;
            } else {
                std::putchar((int)c);
            }
        }
        if (dut.print_int_sel) {
            last_int = (uint32_t)dut.print_int_data;
            saw_int = true;
            std::printf("%x\n", (unsigned)last_int);
            if (dump_events) {
                auto& evA =
                    tb.top.rootp->soc__DOT__accel__DOT__ev_mem__DOT__memA;
                for (uint32_t i = 0; i < last_int; i++)
                    std::printf("%x\n", (unsigned)evA[i]);
            }
        }
        if (dut.exit_sel) {
            const uint32_t code = dut.exit_code;
            const uint64_t active = busy + stalled;
            std::fflush(stdout);
            if (active)
                std::fprintf(stderr,
                             "[accel] busy=%llu stalled=%llu  busy=%.1f%%\n",
                             (unsigned long long)busy,
                             (unsigned long long)stalled,
                             100.0 * (double)busy / (double)active);
            if (saw_mark)
                std::fprintf(stderr,
                             "[phase] encode=%llu eval=%llu  eval=%.1f%%\n",
                             (unsigned long long)enc_cycles,
                             (unsigned long long)eval_cycles,
                             100.0 * (double)eval_cycles /
                                 (double)(enc_cycles + eval_cycles));
            if (saw_int)
                std::fprintf(stderr, "[sim] prediction = %u\n",
                             (unsigned)last_int);
            std::fprintf(stderr, "[sim] EXIT code=%u after %llu cycles\n",
                         code, (unsigned long long)(tb.cycle() - start));
            return (int)code;
        }
    }

    std::fflush(stdout);
    std::fprintf(stderr, "[sim] TIMEOUT after %llu cycles (no EXIT write)\n",
                 (unsigned long long)max_cycles);
    return 124;
}
