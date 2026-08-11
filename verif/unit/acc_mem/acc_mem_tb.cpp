// acc_mem holds 128 x 32-bit accumulators as registers, accessed four at a
// time by group index. The read is combinational because the drain does
// read-modify-write in a single cycle.
//
// Two address ports: word_cnt_q addresses the write/clear, word_cnt addresses
// the read. 

// ctrl encoding comes from accel_defs.vh.
#include "Vacc_mem.h"
#include "tb_harness.h"

static const int CTRL_IDLE  = 0;   // 2'b00
static const int CTRL_WRITE = 1;   // 2'b01
static const int CTRL_CLEAR = 2;   // 2'b10
static const int CTRL_UNUSED = 3;  // 2'b11

static void write_group(Testbench<Vacc_mem>& tb, int group,
                        uint32_t l0, uint32_t l1, uint32_t l2, uint32_t l3) {
    tb.top.word_cnt_q = group;
    tb.top.ctrl       = CTRL_WRITE;
    tb.top.acc_wdata[0] = l0;
    tb.top.acc_wdata[1] = l1;
    tb.top.acc_wdata[2] = l2;
    tb.top.acc_wdata[3] = l3;
    tb.tick();
    tb.top.ctrl = CTRL_IDLE;
}

// combinational read: address in, data out, no tick
static uint32_t read_lane(Testbench<Vacc_mem>& tb, int group, int lane) {
    tb.top.word_cnt = group;
    tb.settle();
    return tb.top.acc_rdata[lane];
}

int main() {
    Testbench<Vacc_mem> tb("verif/build/acc_mem/acc_mem.vcd");
    auto& dut = tb.top;

    dut.ctrl       = CTRL_IDLE;
    dut.word_cnt   = 0;
    dut.word_cnt_q = 0;
    tb.settle();

    // write group 31 (entries 124..127) with four distinct values, so a
    // swapped or shifted lane shows up
    write_group(tb, 31, 0x11111111, 0x22222222, 0x33333333, 0x44444444);
    CHECK_EQ(read_lane(tb, 31, 0), 0x11111111u, "lane 0 should land in its own entry");
    CHECK_EQ(read_lane(tb, 31, 1), 0x22222222u, "lane 1 should land in its own entry");
    CHECK_EQ(read_lane(tb, 31, 2), 0x33333333u, "lane 2 should land in its own entry");
    CHECK_EQ(read_lane(tb, 31, 3), 0x44444444u, "lane 3 should land in its own entry");

    // anything that isn't WRITE or CLEAR must leave memory alone
    dut.word_cnt_q   = 31;
    dut.word_cnt     = 31;
    dut.acc_wdata[0] = 0xDEADBEEF;
    dut.ctrl = CTRL_IDLE;
    tb.tick();
    CHECK_EQ(read_lane(tb, 31, 0), 0x11111111u, "ctrl=idle must not write");

    // a second group holds different data, and the read follows word_cnt with no clock edge in between
    write_group(tb, 0, 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD);

    dut.word_cnt = 0;
    tb.settle();
    CHECK_EQ(dut.acc_rdata[0], 0xAAAAAAAAu, "group 0 should read its own data");
    dut.word_cnt = 31;
    tb.settle();
    CHECK_EQ(dut.acc_rdata[0], 0x11111111u, "read should follow word_cnt without a tick");

    // the flush cycle: the drain's last write lands in group 31 while the sweep reads group 0 in the same cycle. 
    dut.word_cnt_q   = 31;
    dut.word_cnt     = 0;
    dut.acc_wdata[0] = 0x55555555;
    dut.acc_wdata[1] = 0x66666666;
    dut.acc_wdata[2] = 0x77777777;
    dut.acc_wdata[3] = 0x88888888;
    dut.ctrl = CTRL_WRITE;
    tb.settle();
    CHECK_EQ(dut.acc_rdata[0], 0xAAAAAAAAu, "read must follow word_cnt while the write targets word_cnt_q");
    tb.tick();
    dut.ctrl = CTRL_IDLE;
    CHECK_EQ(read_lane(tb, 31, 0), 0x55555555u, "write should land at word_cnt_q, not word_cnt");
    CHECK_EQ(read_lane(tb, 0, 0), 0xAAAAAAAAu, "the group being read must be untouched");

    // clear zeroes the group addressed by word_cnt_q, and only on the clock edge
    dut.word_cnt_q = 31;
    dut.word_cnt   = 31;
    dut.ctrl       = CTRL_CLEAR;
    tb.settle();
    CHECK_EQ(dut.acc_rdata[0], 0x55555555u, "clear should not take effect before the posedge");
    tb.tick();
    dut.ctrl = CTRL_IDLE;
    CHECK_EQ(read_lane(tb, 31, 0), 0x00000000u, "clear should zero the group on the posedge");
    CHECK_EQ(read_lane(tb, 31, 3), 0x00000000u, "clear should zero every lane");
    CHECK_EQ(read_lane(tb, 0, 0), 0xAAAAAAAAu, "clear must only touch the addressed group");

    TRACE_LINE("cycles=%llu", (unsigned long long)tb.cycle());
    return tb_report();
}
