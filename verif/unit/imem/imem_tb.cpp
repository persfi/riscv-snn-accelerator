
// sentinel values for this tb live in vectors.hex, loaded directly into the
// DUT below via load_hex() (bypasses imem.v's own initial $readmemh, which
// still reads program.hex for the FPGA/elaboration path but isn't what
// this test depends on)
#include "Vimem.h"
#include "Vimem___024root.h"
#include "tb_harness.h"

int main() {
    Testbench<Vimem> tb("verif/build/imem/imem.vcd");
    auto& dut = tb.top;

    // Depth comes from the Verilated array so the probe addresses below track
    // imem.v's DEPTH param instead of restating it.
    auto& mem = tb.top.rootp->imem__DOT__mem;
    const uint32_t depth = sizeof(mem) / sizeof(mem[0]);
    const uint32_t window = depth * 4;  // byte size of the decoded window
    load_hex(mem, "verif/unit/imem/vectors.hex", depth);
    mem[depth - 1] = 0xabcdefff;  // boundary sentinel: index depends on DEPTH

    dut.addr = 0x0;
    tb.settle();
    TRACE_LINE("addr=%08x inst=%08x", (uint32_t)dut.addr, (uint32_t)dut.inst);
    CHECK_EQ(dut.inst, 0x00000008, "imem should read index 0 at addr 0x0");

    dut.addr = 0x4;
    tb.settle();
    TRACE_LINE("addr=%08x inst=%08x", (uint32_t)dut.addr, (uint32_t)dut.inst);
    CHECK_EQ(dut.inst, 0x00000000, "imem should read index 1 at addr 0x4");

    dut.addr = 0x8;
    tb.settle();
    TRACE_LINE("addr=%08x inst=%08x", (uint32_t)dut.addr, (uint32_t)dut.inst);
    CHECK_EQ(dut.inst, 0x00000015, "imem should read index 2 at addr 0x8");

    dut.addr = 0x9;
    tb.settle();
    TRACE_LINE("addr=%08x inst=%08x", (uint32_t)dut.addr, (uint32_t)dut.inst);
    CHECK_EQ(dut.inst, 0x00000015, "imem should be able to truncate the lower bits of the addr to multiples of 4 to read words for instructions");

    dut.addr = window + 0xa; 
    tb.settle();
    TRACE_LINE("addr=%08x inst=%08x", (uint32_t)dut.addr, (uint32_t)dut.inst);
    CHECK_EQ(dut.inst, 0x00000015, "imem should be able to truncate the upper bits of the addr to multiples of 4 to read words for instructions");


    dut.addr = window - 4; 
    tb.settle();
    TRACE_LINE("addr=%08x inst=%08x", (uint32_t)dut.addr, (uint32_t)dut.inst);
    CHECK_EQ(dut.inst, 0xabcdefff, "imem should read index DEPTH-1 at the top of its window");

    return tb_report();
}
