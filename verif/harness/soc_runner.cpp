#include "Vsoc.h"
#include "Vsoc___024root.h"
#include "tb_harness.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <program.hex> [max_cycles]\n", argv[0]);
        return 2;
    }
    const char* hex_path = argv[1];
    const uint64_t max_cycles =
        (argc > 2) ? std::strtoull(argv[2], nullptr, 0) : 200000;

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

    // Reset one cycle, then release. The computation baseline is measured from
    // release, so snapshot the cycle count there.
    dut.rst = 1;
    tb.tick();
    dut.rst = 0;
    const uint64_t start = tb.cycle();

    // Single-cycle core: print_sel/exit_sel are combinational decodes that hold
    // for exactly one settled cycle per store, so sampling once per tick catches
    // each MMIO write exactly once (same observe-after-tick shape the riscv-tests
    // runner uses on the tohost word).
    while (tb.cycle() < max_cycles) {
        tb.tick();
        if (dut.print_sel) std::putchar((int)dut.print_data);
        if (dut.exit_sel) {
            const uint32_t code = dut.exit_code;
            std::fflush(stdout);
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
