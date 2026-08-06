// ev_mem is the input spike queue: two 784 x 10-bit banks so the host can fill one while the drain reads the other. It has two seperate port (wr and rd) so that the accelerator can read and write without overlapping
#include "Vev_mem.h"
#include "tb_harness.h"

static void write_at(Testbench<Vev_mem>& tb, int bank, uint16_t addr, uint16_t data) {
    tb.top.wr_bank = bank;
    tb.top.wr_addr = addr;
    tb.top.wr_data = data;
    tb.top.we      = 1;
    tb.tick();
    tb.top.we = 0;
}

static uint16_t read_at(Testbench<Vev_mem>& tb, int bank, uint16_t addr) {
    tb.top.rd_bank = bank;
    tb.top.rd_addr = addr;
    tb.settle();
    return tb.top.rd_data;
}

int main() {
    Testbench<Vev_mem> tb("verif/build/ev_mem/ev_mem.vcd");
    auto& dut = tb.top;

    dut.we      = 0;
    dut.wr_bank = 0;
    dut.wr_addr = 0;
    dut.wr_data = 0;
    dut.rd_bank = 0;
    dut.rd_addr = 0;
    tb.settle();

    // write bank A and prove it lands on the edge
    dut.rd_bank = 0;
    dut.rd_addr = 5;
    dut.wr_bank = 0;
    dut.wr_addr = 5;
    dut.wr_data = 0x111;
    dut.we      = 1;
    tb.settle();
    CHECK_EQ(dut.rd_data, 0x000, "write should not land before the posedge");
    tb.tick();
    dut.we = 0;
    tb.settle();
    CHECK_EQ(dut.rd_data, 0x111, "bank A be written at posedge");

    // write bank B at the same address while the read port stays on bank A.
    dut.rd_bank = 0;
    dut.rd_addr = 5;
    dut.wr_bank = 1;
    dut.wr_addr = 5;
    dut.wr_data = 0x222;
    dut.we      = 1;
    tb.settle();
    CHECK_EQ(dut.rd_data, 0x111, "reading A must be unaffected during a B write");
    tb.tick();
    dut.we = 0;
    tb.settle();
    CHECK_EQ(dut.rd_data, 0x111, "the B write must not reach bank A");

    // same address, the other bank: read should be combinational
    dut.rd_bank = 1;
    tb.settle();
    CHECK_EQ(dut.rd_data, 0x222, "bank B holds its own data at the same address and read without tick");

    // we=0 doesn't write to memory
    dut.wr_bank = 0;
    dut.wr_addr = 5;
    dut.wr_data = 0x3FF;
    dut.we      = 0;
    tb.tick();
    CHECK_EQ(read_at(tb, 0, 5), 0x111, "we=0 must not write");

    // test boundaries
    write_at(tb, 0, 783, 0x0AA);
    write_at(tb, 1, 783, 0x0BB);
    CHECK_EQ(read_at(tb, 0, 783), 0x0AA, "bank A addr 783 should be writable");
    CHECK_EQ(read_at(tb, 1, 783), 0x0BB, "bank B addr 783 should be writable");

    TRACE_LINE("cycles=%llu", (unsigned long long)tb.cycle());
    return tb_report();
}
