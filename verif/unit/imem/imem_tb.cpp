
//index[] values in program.hex
/* program.hex testing values for this tb:

0x8
0x0
0x15
@3ff // jumps to index 1023 to test boundaries
0xabcdefff

*/
#include "Vimem.h"
#include "tb_harness.h"

int main() {
    Testbench<Vimem> tb("verif/build/imem/imem.vcd");
    auto& dut = tb.top;

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

    dut.addr = 0x100a; //should decode to the same as 0xa which should be truncated to 0x8
    tb.settle();
    TRACE_LINE("addr=%08x inst=%08x", (uint32_t)dut.addr, (uint32_t)dut.inst);
    CHECK_EQ(dut.inst, 0x00000015, "imem should be able to truncate the upper bits of the addr to multiples of 4 to read words for instructions");


    dut.addr = 0xffc; // byte addr of word index 1023 = DEPTH-1, the boundary
    tb.settle();
    TRACE_LINE("addr=%08x inst=%08x", (uint32_t)dut.addr, (uint32_t)dut.inst);
    CHECK_EQ(dut.inst, 0xabcdefff, "imem should read index DEPTH-1 at the top of its window");

    return tb_report();
}
