
// sentinel values for this tb live in vectors.hex, loaded directly into the
// DUT below via load_hex() (bypasses dmem.v's own initial $readmemh, which
// still reads data.hex for the FPGA/elaboration path but isn't what
// this test depends on). Same index-decode logic as imem, so this reuses
// imem's already-proven vectors rather than authoring new ones.
#include "Vdmem.h"
#include "Vdmem___024root.h"
#include "tb_harness.h"

int main() {
    Testbench<Vdmem> tb("verif/build/dmem/dmem.vcd");
    auto& dut = tb.top;

    // Depth comes from the Verilated array so the probe addresses below track
    // dmem.v's DEPTH param instead of restating it.
    auto& mem = tb.top.rootp->dmem__DOT__mem;
    const uint32_t depth = sizeof(mem) / sizeof(mem[0]);
    const uint32_t window = depth * 4;  // byte size of the decoded window
    load_hex(mem, "verif/unit/dmem/vectors.hex", depth);
    mem[depth - 1] = 0xabcdefff;  // boundary sentinel: index depends on DEPTH

    dut.addr = 0x0;
    tb.settle();
    TRACE_LINE("addr=%08x rdata=%08x", (uint32_t)dut.addr, (uint32_t)dut.rdata);
    CHECK_EQ(dut.rdata, 0x00000008, "dmem should read index 0 at addr 0x0");

    dut.addr = 0x4;
    tb.settle();
    TRACE_LINE("addr=%08x rdata=%08x", (uint32_t)dut.addr, (uint32_t)dut.rdata);
    CHECK_EQ(dut.rdata, 0x00000000, "dmem should read index 1 at addr 0x4");

    dut.addr = 0x8;
    tb.settle();
    TRACE_LINE("addr=%08x rdata=%08x", (uint32_t)dut.addr, (uint32_t)dut.rdata);
    CHECK_EQ(dut.rdata, 0x00000015, "dmem should read index 2 at addr 0x8");

    dut.addr = 0x9;
    tb.settle();
    TRACE_LINE("addr=%08x rdata=%08x", (uint32_t)dut.addr, (uint32_t)dut.rdata);
    CHECK_EQ(dut.rdata, 0x00000015, "dmem should be able to truncate the lower bits of the addr to multiples of 4 to read words");

    dut.addr = window + 0xa; 
    tb.settle();
    TRACE_LINE("addr=%08x rdata=%08x", (uint32_t)dut.addr, (uint32_t)dut.rdata);
    CHECK_EQ(dut.rdata, 0x00000015, "dmem should be able to truncate the upper bits of the addr to multiples of 4 to read words");

    dut.addr = window - 4; 
    tb.settle();
    TRACE_LINE("addr=%08x rdata=%08x", (uint32_t)dut.addr, (uint32_t)dut.rdata);
    CHECK_EQ(dut.rdata, 0xabcdefff, "dmem should read index DEPTH-1 at the top of its window");

    return tb_report();
}
