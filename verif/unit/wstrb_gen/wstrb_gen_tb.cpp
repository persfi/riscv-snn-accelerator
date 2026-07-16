#include "Vwstrb_gen.h"
#include "tb_harness.h"

constexpr uint8_t SB = 0b000;
constexpr uint8_t SH = 0b001;
constexpr uint8_t SW = 0b010;

int main() {
    Testbench<Vwstrb_gen> tb("verif/build/wstrb_gen/wstrb_gen.vcd");
    auto& dut = tb.top;

    auto chk = [&](uint8_t expected, const char* msg) {
        tb.settle();
        TRACE_LINE("funct3=%u addr_lo=%u wstrb=%01x",
                   (unsigned)dut.funct3, (unsigned)dut.addr_lo,
                   (unsigned)dut.wstrb);
        CHECK_EQ(dut.wstrb, expected, msg);
    };

    // sb: one-hot byte lane
    dut.funct3 = SB;
    dut.addr_lo = 0; chk(0b0001, "sb lane0");
    dut.addr_lo = 1; chk(0b0010, "sb lane1");
    dut.addr_lo = 2; chk(0b0100, "sb lane2");
    dut.addr_lo = 3; chk(0b1000, "sb lane3");

    // sh: only addr_lo[1] selects the half
    dut.funct3 = SH;
    dut.addr_lo = 0; chk(0b0011, "sh low half");
    dut.addr_lo = 1; chk(0b0011, "sh addr_lo[0] ignored -> still low half");
    dut.addr_lo = 2; chk(0b1100, "sh high half");
    dut.addr_lo = 3; chk(0b1100, "sh addr_lo[0] ignored -> still high half");

    // sw: whole word, addr_lo irrelevant
    dut.funct3 = SW;
    dut.addr_lo = 0; chk(0b1111, "sw lane0: whole word");
    dut.addr_lo = 1; chk(0b1111, "sw: addr_lo ignored");
    dut.addr_lo = 2; chk(0b1111, "sw: addr_lo ignored");
    dut.addr_lo = 3; chk(0b1111, "sw: addr_lo ignored");

    // default (unlisted funct3): no write
    dut.funct3 = 0b011;
    dut.addr_lo = 0; chk(0b0000, "funct3=011 default: no write");
    dut.addr_lo = 1; chk(0b0000, "funct3=011 default: addr_lo ignored");


    return tb_report();
}
