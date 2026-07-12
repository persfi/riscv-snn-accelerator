#include "Vcore.h"
#include "Vcore___024root.h"
#include "tb_harness.h"

int main() {
    Testbench<Vcore> tb("verif/build/core/core.vcd");
    auto& dut = tb.top;
    auto& regs = tb.top.rootp->core__DOT__regfile__DOT__registers;
    auto& dmem_arr = tb.top.rootp->core__DOT__dmem__DOT__mem;
    auto& imem_arr = tb.top.rootp->core__DOT__imem__DOT__mem;

    //tick + print trace, no need for writing trace lines manually
    auto run = [&](int n) {
        for (int i = 0; i < n; ++i) tb.tick();
        TRACE_LINE("cycle=%llu pc=%08x inst=%08x unknown_op=%d",
                   (unsigned long long)tb.cycle(), (uint32_t)dut.pc_q,
                   (uint32_t)dut.inst, (int)dut.unknown_op);
    };//cycle + pc + inst + unknown_op

    //fetch: pc + imem only
    load_hex(imem_arr, "verif/unit/core/fetch_vectors.hex", 1024);

    dut.rst = 1;
    run(1);
    CHECK_EQ(dut.pc_q, 0x00000000, "pc_q should reset to 0 after rst=1");
    CHECK_EQ(dut.inst, 0x00000008, "imem should read inst addr 0x0");

    dut.rst = 0;
    run(1);
    CHECK_EQ(dut.pc_q, 0x00000004, "pc_q should advance 4 after every clk tick");
    CHECK_EQ(dut.inst, 0x00000000, "imem should read inst at addr pc_q");

    run(1);
    CHECK_EQ(dut.pc_q, 0x00000008, "pc_q should advance 4 after every clk tick");
    CHECK_EQ(dut.inst, 0x00000015, "imem should read inst at addr pc_q");

    dut.rst = 1;
    run(1);
    dut.rst = 0;
    CHECK_EQ(dut.pc_q, 0x00000000, "pc_q should reset to 0 after rst=1 in mid cycle");
    CHECK_EQ(dut.inst, 0x00000008, "imem should read inst addr 0x0");

    //lw: full datapath, control decode -> regfile -> imm_gen -> alu -> dmem -> writeback
    //lw_vectors.hex is the instruction stream (imem); data.hex is what that instruction should load (dmem)
    //(renamed from lw_data.hex as its reusable not lw-specific)
    load_hex(imem_arr, "verif/unit/core/lw_vectors.hex", 1024);
    load_hex(dmem_arr, "verif/unit/core/data.hex", 1024);

    tb.settle(); //updates sim to new inst in lw_vectors.hex
    TRACE_LINE("cycle=%llu pc=%08x inst=%08x", (unsigned long long)tb.cycle(), (uint32_t)dut.pc_q, (uint32_t)dut.inst);
    CHECK_EQ(dut.pc_q, 0x00000000, "pc_q should still be 0 right after loading lw hex, before any tick");
    CHECK_EQ(dut.inst, 0x00002283, "inst should combinationally read the first lw (lw x5,0(x0))");
    // zero-init dependent: this only passes because Verilator zero-inits registers[5], it might fail in the future.
    CHECK_REG(5, 0x00000000, "x5 should still be 0 before any clock edge commits the write");

    run(1);
    CHECK_EQ(dut.pc_q, 0x00000004, "pc_q should advance to 4 after executing lw x5,0(x0)");
    CHECK_EQ(dut.inst, 0x00402303, "inst should now read the second lw (lw x6,4(x0))");
    CHECK_REG(5, 0x00000ccc, "x5 should hold dmem[0] after lw x5,0(x0) commits");

    run(1);
    CHECK_EQ(dut.pc_q, 0x00000008, "pc_q should advance to 8 after executing lw x6,4(x0)");
    CHECK_EQ(dut.inst, 0x00532383, "inst should now read the third lw (lw x7,5(x6))");
    CHECK_REG(6, 0x00000004, "x6 should hold dmem[1] after lw x6,4(x0) commits");

    // lw x7,5(x6): x6=4 is a nonzero base register
    // addr 9 (4+5) truncates to word index 2. testing nonzero-rs1 address and misaligned-address truncation together

    run(1);
    CHECK_EQ(dut.pc_q, 0x0000000c, "pc_q should advance to 0xc after executing lw x7,5(x6)");
    CHECK_EQ(dut.inst, 0x00628433, "inst should now read the add (add x8,x5,x6):an opcode control.v doesn't decode yet");
    CHECK_REG(7, 0x00000009, "x7 should hold dmem[2] (addr 9 truncated to word index 2) after lw x7,5(x6) commits");

    dut.rst = 1;
    run(1);
    dut.rst = 0;
    CHECK_EQ(dut.pc_q, 0x00000000, "pc_q should reset to 0");
    CHECK_EQ(dut.inst, 0x00002283, "inst should re-fetch the first lw since pc_q returned to 0");
    CHECK_EQ(dut.unknown_op, 0, "unknown_op should drop back to 0 once a recognized opcode (lw) is fetched again");

    //sw: full datapath, control decode -> regfile -> imm_gen -> alu -> dmem write -> back to dmem read
    //sw_vectors.hex is the instruction stream (imem).
    load_hex(imem_arr, "verif/unit/core/sw_vectors.hex", 1024);
    load_hex(dmem_arr, "verif/unit/core/data.hex", 1024);

    tb.settle(); //updates sim to new inst in sw_vectors.hex
    run(2); //load to x2 = a, x3 = 4

    run(2);
    CHECK_MEM(6, 0x0000000a, "sw x2,25(x0) should write x2 (0xa) into dmem[6], addr 25 truncates to word index 6");
    CHECK_REG(4, 0x0000000a, "lw x4,24(x0) should read back what was just stored, addr 24 also truncates to word index 6");

    run(2);
    CHECK_MEM(6, 0x00000004, "sw x3,23(x3) (nonzero base) should overwrite dmem[6] with x3 (4), addr 4+23=27 also truncates to word index 6");
    CHECK_REG(4, 0x00000004, "lw x4,24(x0) should read back the overwritten value");
    CHECK_REG(2, 0x0000000a, "x2 should still hold its value from lw as rd_we stayed 0.");

    dut.rst = 1;
    run(1);
    dut.rst = 0;
    CHECK_MEM(6, 0x00000004, "dmem[6] should survive rst (dmem.v has no reset path)");

    //r-type: full datapath, control decode -> regfile -> alu (funct7[5]/funct3 passthrough) -> writeback
    //rtype_vectors.hex is the instruction stream (imem). data.hex (reused) preloads dmem so lw can seed x1,x2,x5.
    load_hex(imem_arr, "verif/unit/core/rtype_vectors.hex", 1024);
    load_hex(dmem_arr, "verif/unit/core/data.hex", 1024);

    tb.settle(); //updates sim to new inst in rtype_vectors.hex

    run(1);
    run(1);

    run(1);
    CHECK_REG(3, 0x0000000d, "add x3,x1,x2 = 4+9 = 13; funct3=000,funct7[5]=0 -> ALU_ADD");
    run(1);
    CHECK_REG(4, 0xfffffffb, "sub x4,x1,x2 = 4-9 = -5; same funct3=000 as add, funct7[5]=1 -> ALU_SUB (proves funct7[5] threads through) and rs2 and rs1 are properly pass forward");

    run(1);
    run(1);
    CHECK_REG(6, 0x00000008, "and x6,x2,x5 = 9 & 10 = 8");

    run(1);
    CHECK_REG(7, 0x0fffffff, "srl x7,x4,x1: 0xfffffffb >> 4 logical, zero-filled (funct7[5]=0)");

    run(1);
    CHECK_REG(8, 0xffffffff, "sra x8,x4,x1: 0xfffffffb >>> 4 arithmetic, sign-filled (funct7[5]=1)");
    CHECK_EQ(dut.unknown_op, 1, "lui isn't wired yet, unknown_op should go high as soon as it's fetched");

    run(1);
    CHECK_REG(6, 0x00000008, "x6 should still hold and's result (8) as in the unknown_op category, rd_we = 0");

    return tb_report();
}
