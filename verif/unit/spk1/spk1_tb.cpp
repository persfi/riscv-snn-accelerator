// spk1 is the hidden-spike queue: it stores the idx of spiking events in the hidden layer
#include "Vspk1.h"
#include "tb_harness.h"

static void write_at(Testbench<Vspk1>& tb, uint8_t addr, uint8_t data) {
    tb.top.addr    = addr;
    tb.top.wr_data = data;
    tb.top.we      = 1;
    tb.tick();
    tb.top.we = 0;
}

static uint8_t read_at(Testbench<Vspk1>& tb, uint8_t addr) {
    tb.top.addr = addr;
    tb.settle();
    return tb.top.rd_data;
}

int main() {
    Testbench<Vspk1> tb("verif/build/spk1/spk1.vcd");
    auto& dut = tb.top;

    dut.we      = 0;
    dut.addr    = 0;
    dut.wr_data = 0;
    tb.settle();

    // write lands on the edge
    dut.addr    = 0;
    dut.wr_data = 34;
    dut.we      = 1;
    tb.settle();
    CHECK_EQ(dut.rd_data, 0, "write should not land before the posedge");
    tb.tick();
    dut.we = 0;
    CHECK_EQ(read_at(tb, 0), 34, "write should land on the posedge");

    // top of the range
    write_at(tb, 127, 55);
    CHECK_EQ(read_at(tb, 127), 55, "addr 127 should be writable");

    // combinational read: jump between two addresses with no tick
    dut.addr = 0;
    tb.settle();
    CHECK_EQ(dut.rd_data, 34, "rd_data should still read 34");
    dut.addr = 127;
    tb.settle();
    CHECK_EQ(dut.rd_data, 55, "rd_data should follow addr with no tick");

    // we=0 doesn't write to memory
    dut.addr    = 0;
    dut.wr_data = 99;
    dut.we      = 0;
    tb.tick();
    CHECK_EQ(read_at(tb, 0), 34, "we=0 must not write");

    TRACE_LINE("cycles=%llu", (unsigned long long)tb.cycle());
    return tb_report();
}
