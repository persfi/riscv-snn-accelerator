
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
    load_hex(tb.top.rootp->dmem__DOT__mem, "verif/unit/dmem/vectors.hex", 1024);

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

    dut.addr = 0x100a; //should decode to the same as 0xa which should be truncated to 0x8
    tb.settle();
    TRACE_LINE("addr=%08x rdata=%08x", (uint32_t)dut.addr, (uint32_t)dut.rdata);
    CHECK_EQ(dut.rdata, 0x00000015, "dmem should be able to truncate the upper bits of the addr to multiples of 4 to read words");

    dut.addr = 0xffc; // byte addr of word index 1023 = DEPTH-1, the boundary
    tb.settle();
    TRACE_LINE("addr=%08x rdata=%08x", (uint32_t)dut.addr, (uint32_t)dut.rdata);
    CHECK_EQ(dut.rdata, 0xabcdefff, "dmem should read index DEPTH-1 at the top of its window");

    return tb_report();
}
