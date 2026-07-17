// Runs one official riscv-tests rv32ui vector on the core and reports pass/fail
// via the tohost protocol.
//
//   usage: core_sim <program.hex> <tohost_byte_addr_hex> [max_cycles]
//
// Infrastructure only: the pass/fail *decision* lives inside the upstream
// test's own self-checks (each sub-test branches to fail on a wrong result).
// This just mirror-loads the image, watches the tohost word, and ends the sim.
//
// Mirror memory model: the same image is loaded into BOTH imem and dmem, so a
// Harvard core (separate instruction/data memories, no shared backing) can run
// a single unified test image -- a fetch finds code in imem, a load finds data
// in dmem, because both hold the whole image. See docs/DESIGN.md.

#include "Vcore.h"
#include "Vcore___024root.h"
#include "tb_harness.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr,
                     "usage: %s <program.hex> <tohost_addr_hex> [max_cycles]\n",
                     argv[0]);
        return 2;
    }
    const char* hex_path = argv[1];
    const uint32_t tohost_addr = (uint32_t)std::strtoul(argv[2], nullptr, 16);
    const uint64_t max_cycles =
        (argc > 3) ? std::strtoull(argv[3], nullptr, 0) : 200000;

    Testbench<Vcore> tb("verif/build/core-sim/run.vcd");
    auto& dut  = tb.top;
    auto& imem = tb.top.rootp->core__DOT__imem__DOT__mem;
    auto& dmem = tb.top.rootp->core__DOT__dmem__DOT__mem;

    // Depth is read straight from the Verilated array so it always tracks the
    // RTL's DEPTH parameter -- no second place to keep in sync.
    const size_t depth = sizeof(dmem) / sizeof(dmem[0]);
    const uint32_t tohost_idx = tohost_addr / 4;
    if (tohost_idx >= depth) {
        std::fprintf(stderr,
                     "tohost addr 0x%x (word %u) exceeds mem depth %zu; "
                     "raise the core's DEPTH for this test\n",
                     tohost_addr, tohost_idx, depth);
        return 2;
    }

    // Mirror the same image into both memories.
    load_hex(imem, hex_path, depth);
    load_hex(dmem, hex_path, depth);

    // Reset for one cycle, then release.
    dut.rst = 1;
    tb.tick();
    dut.rst = 0;

    // Run until the test writes tohost (non-zero), or we hit the safety cap.
    uint32_t tohost = 0;
    while (tb.cycle() < max_cycles) {
        tb.tick();
        tohost = dmem[tohost_idx];
        if (tohost != 0) break;
    }

    if (tohost == 0) {
        std::printf("TIMEOUT after %llu cycles (tohost never written)\n",
                    (unsigned long long)max_cycles);
        return 1;
    }
    if (tohost == 1) {
        std::printf("PASS (%llu cycles)\n", (unsigned long long)tb.cycle());
        return 0;
    }
    std::printf("FAIL sub-test %u (%llu cycles)\n",
                tohost >> 1, (unsigned long long)tb.cycle());
    return 1;
}
