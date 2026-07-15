#include "Vload_ext.h"
#include "tb_harness.h"

constexpr uint8_t LB  = 0b000;
constexpr uint8_t LH  = 0b001;
constexpr uint8_t LW  = 0b010;
constexpr uint8_t LBU = 0b100;
constexpr uint8_t LHU = 0b101;

int main() {
    Testbench<Vload_ext> tb("verif/build/load_ext/load_ext.vcd");
    auto& dut = tb.top;

    // settle, optionally trace (TRACE=1), then assert load_data
    auto chk = [&](uint32_t expected, const char* msg) {
        tb.settle();
        TRACE_LINE("funct3=%u addr_lo=%u load_data=%08x",
                   (unsigned)dut.funct3, (unsigned)dut.addr_lo,
                   (uint32_t)dut.load_data);
        CHECK_EQ(dut.load_data, expected, msg);
    };

    // Word under test: bytes {ab, cd, 02, 01}, halves {abcd, 0201}.
    // Bytes 0-1 positive, bytes 2-3 negative; low half positive, high half negative.
    dut.rdata = 0xabcd0201;

    // lb: sign-extend selected byte
    dut.funct3 = LB;
    dut.addr_lo = 0; chk(0x00000001, "lb lane0: +0x01");
    dut.addr_lo = 1; chk(0x00000002, "lb lane1: +0x02");
    dut.addr_lo = 2; chk(0xffffffcd, "lb lane2: -0xcd sign-extended");
    dut.addr_lo = 3; chk(0xffffffab, "lb lane3: -0xab sign-extended");

    // lh: sign-extend selected half (only addr_lo[1] selects the half)
    dut.funct3 = LH;
    dut.addr_lo = 0; chk(0x00000201, "lh low half: +0x0201 zero-filled");
    dut.addr_lo = 1; chk(0x00000201, "lh addr_lo[0] ignored -> still low half");
    dut.addr_lo = 2; chk(0xffffabcd, "lh high half: -0xabcd sign-extended");
    dut.addr_lo = 3; chk(0xffffabcd, "lh addr_lo[0] ignored -> still high half");

    // lw: whole word, addr_lo irrelevant
    dut.funct3 = LW;
    dut.addr_lo = 0; chk(0xabcd0201, "lw lane0: whole word");
    dut.addr_lo = 1; chk(0xabcd0201, "lw: addr_lo ignored");
    dut.addr_lo = 2; chk(0xabcd0201, "lw: addr_lo ignored");
    dut.addr_lo = 3; chk(0xabcd0201, "lw: addr_lo ignored");

    // lbu: zero-extend selected byte
    dut.funct3 = LBU;
    dut.addr_lo = 0; chk(0x00000001, "lbu lane0: 0x01 zero-extended");
    dut.addr_lo = 1; chk(0x00000002, "lbu lane1: 0x02 zero-extended");
    dut.addr_lo = 2; chk(0x000000cd, "lbu lane2: 0xcd zero-extended");
    dut.addr_lo = 3; chk(0x000000ab, "lbu lane3: 0xab zero-extended");

    // lhu: zero-extend selected half
    dut.funct3 = LHU;
    dut.addr_lo = 0; chk(0x00000201, "lhu low half: 0x0201 zero-extended");
    dut.addr_lo = 1; chk(0x00000201, "lhu addr_lo[0] ignored -> low half");
    dut.addr_lo = 2; chk(0x0000abcd, "lhu high half: 0xabcd zero-extended");
    dut.addr_lo = 3; chk(0x0000abcd, "lhu addr_lo[0] ignored -> high half");

    // default (unlisted funct3): pass raw rdata through
    dut.funct3 = 0b011;
    dut.addr_lo = 0; chk(0xabcd0201, "funct3=011 default: raw rdata");
    dut.addr_lo = 1; chk(0xabcd0201, "funct3=011 default: addr_lo ignored");

    return tb_report();
}
