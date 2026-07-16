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
    // CHECK_EQ(dut.unknown_op, 1, "lui isn't wired yet, unknown_op should go high as soon as it's fetched");

    run(1);
    // CHECK_REG(6, 0x00000008, "x6 should still hold and's result (8) as in the unknown_op category, rd_we = 0");

    //i-type: full datapath, control decode -> regfile -> imm_gen -> alu (lbit-forced funct7[5]/funct3 passthrough) -> writeback
    //itype_vectors.hex is the instruction stream (imem). no dmem needed:every source value comes from an addi immediate.

    load_hex(imem_arr, "verif/unit/core/itype_vectors.hex", 1024);

    dut.rst = 1;
    run(1);
    dut.rst = 0;
    tb.settle(); //updates sim to new inst in itype_vectors.hex

    run(1);
    CHECK_REG(5, 0x00000400, "if lbit weren't forced to 0 for non-shift OP_IMM, this would hit ALU_SUB and give 0xfffffc00");

    run(1);
    CHECK_REG(4, 0xfffffffb, "x4 should be -5");

    run(1);
    CHECK_REG(5, 0x00000403, "rd==rs1, proves the combinational read (old x5) and non-blocking write (new x5) don't race in the same cycle");

    run(1);
    CHECK_REG(6, 0x0fffffff, "0xfffffffb >> 4 logical, filling 0s. Proves it's reading the correct funct7[5] compared to srai (next check)");

    run(1);
    CHECK_REG(7, 0xffffffff, "0xfffffffb >>> 4 arithmetic, sign-filled");
    // CHECK_EQ(dut.unknown_op, 1, "lui isn't wired yet, unknown_op should go high as soon as it's fetched");

    run(1);
    // CHECK_REG(5, 0x00000403, "x5 should still hold 0x403 from the previous addi as in the unknown_op category, rd_we = 0");

    //b-type: full datapath, control decode -> branch (comparison already unit-tested in branch_tb.cpp) > mux_pc/pc_target -> pc. this section only proves the wiring: taken/not-taken, forward skip, backward jump, and that a taken branch actually skips the instruction it jumps over.
    //btype_vectors.hex is the instruction stream (imem). no dmem needed: every value comes from addi immediates.

    load_hex(imem_arr, "verif/unit/core/btype_vectors.hex", 1024);

    dut.rst = 1;
    run(1);
    dut.rst = 0;
    tb.settle(); //updates sim to new inst in btype_vectors.hex

    run(1);
    CHECK_REG(1, 0x00000005, "addi x1,x0,5: x1=5");

    run(1);
    CHECK_REG(2, 0x00000005, "addi x2,x0,5: x2=5");

    run(1);
    CHECK_EQ(dut.pc_q, 0x0000000c, "proves that the opcode eval and branch_ctrl works correctly");
    CHECK_REG(5, 0x0000000a, "it should execute add operation");

    run(1);
    CHECK_EQ(dut.pc_q, 0x00000014, "beq x1,x2,target: 5==5, taken, lands on target (0x14), skipping addi x3,x0,1 at 0x10");

    run(1);
    CHECK_REG(3, 0x00000002, "target: addi x3,x0,2: x3=2, confirms the skipped addi x3,x0,1");

    run(1);
    CHECK_EQ(dut.pc_q, 0x0000001c, "bne x1,x2,target2: 5==5, not taken, falls through to 0x1c");

    run(1);
    CHECK_REG(3, 0x00000003, "addi x3,x0,3: x3=3, proves it actually fell through and executed this");

    run(1); 

    run(1);
    CHECK_REG(3, 0x00000005, "target3 (1st pass): addi x3,x3,2 = 3+2 = 5");

    run(1);
    CHECK_EQ(dut.pc_q, 0x00000024, "bge x2,x3,target3 (1st eval): 5>=5, taken, jumps backward to target3 (0x24)");

    run(1);
    CHECK_REG(3, 0x00000007, "target3 (2nd pass): addi x3,x3,2 = 5+2 = 7");

    run(1);
    CHECK_EQ(dut.pc_q, 0x0000002c, "bge x2,x3,target3 (2nd eval): 5>=7 is false, not taken, exits to 0x2c");

    run(1);
    CHECK_REG(4, 0xfffffff6, "addi x4,x0,-10: x4=-10");

    run(1);
    CHECK_EQ(dut.pc_q, 0x00000038, "blt x4,x3,target4: signed -10 < 7, taken, lands on target4 (0x38), skipping target5 at 0x34");

    run(1);
    CHECK_EQ(dut.pc_q, 0x0000003c, "bltu x4,x3,target5: unsigned -10 (0xfffffff6) > 7, not taken");
    CHECK_REG(5, 0x0000000a, "x5 should still hold 10 from the add earlier in this section");

    //j-type: full datapath, control decode -> imm_gen  -> jal target adder -> pc; rd <= pc+4 link via mux_r.
    //jtype_vectors.hex is the instruction stream (imem). data.hex (reused) preloads dmem.
    // reloading dmem to keep this section's inputs independent of whatever earlier sections did to dmem.
    load_hex(imem_arr, "verif/unit/core/jtype_vectors.hex", 1024);
    load_hex(dmem_arr, "verif/unit/core/data.hex", 1024);

    dut.rst = 1;
    run(1);
    dut.rst = 0;
    tb.settle(); //updates sim to new inst in jtype_vectors.hex

    run(1); 
    run(1); 

    run(1); // jump 2052, setting imm[11] (inst[20]) 
    CHECK_EQ(dut.pc_q, 0x0000080c, "pc should jump forward to 0x80c");
    CHECK_REG(0, 0x00000000, "x0 must not get coded with anything but 0");

    run(1);
    // CHECK_EQ(dut.unknown_op, 1, "lui isn't wired yet, unknown_op should go high");
    run(1);
    run(1);
    CHECK_EQ(dut.pc_q, 0x0000000c, "pc should jump back to target2 (0x0c)");

    run(1); 

    run(1);
    CHECK_EQ(dut.pc_q, 0x00000018, "jal x1,target should jump to target (0x18), skipping the addi at 0x14");
    CHECK_REG(1, 0x00000014, "jal x1,target should link x1 = pc+4 = 0x14, proving jal computes the link independent of any register read");

    run(1); 
    CHECK_EQ(dut.pc_q, 0x0000000c, "pc should jump backward to target2 (0x0c)");
    CHECK_REG(2, 0x0000001c, "x2 should be overwritten with link pc+4=0x1c regardless of it's old value");

    run(1); 

    run(1);
    CHECK_EQ(dut.pc_q, 0x00000018, "jal x1,target lands on target again, proving jump is unconditional every time, not just once");
    CHECK_REG(1, 0x00000014, "x1 should jump to pc+4=0x14");

    //jalr: full datapath, control decode -> base-select mux (rs1_data vs pc_q) -> adder -> LSB clear -> pc; rd <= pc+4 link via mux_r.
    //jalr_vectors.hex is the instruction stream (imem). no dmem needed: every source value comes from addi/link registers.
    load_hex(imem_arr, "verif/unit/core/jalr_vectors.hex", 1024);

    dut.rst = 1;
    run(1);
    dut.rst = 0;
    tb.settle(); //updates sim to new inst in jalr_vectors.hex

    run(1); 

    run(1); 
    CHECK_EQ(dut.pc_q, 0x00000010, "jalr x2,12(x1) should truncate the odd sum (17) down to 16, proving the LSB-clear actually fires");
    CHECK_REG(2, 0x00000008, "jalr x2,12(x1) should link x2 = pc+4 = 0x08");

    run(1); 
    CHECK_EQ(dut.pc_q, 0x00000008, "jalr x0,0(x2) should jump to x2+0=0x08, proving it jumps backwards to absolute address");
    CHECK_REG(0, 0x00000000, "jalr x0,0(x2) must not write into x0 , x0 stays 0");

    run(1);

    run(1);
    CHECK_EQ(dut.pc_q, 0x00000014, "jalr x3,17(x3) should jump to (0x14)");
    CHECK_REG(3, 0x00000010, "jalr x3,17(x3) should link x3=pc+4=0x10, proving rs1_data was read before write, no race occured.");

    run(1); 

    run(1);
    CHECK_EQ(dut.pc_q, 0x00000000, "jalr x4,-8(x2) should jump to 8+(-8)=0, proving imm is sign-extended (not zero-extended) through the jalr adder");

    //u-type: full datapath, control decode -> imm_gen (IMM_U, {inst[31:12],12'b0}) -> mux_r (RESULT_LUI) -> writeback.
    //also confirms a genuinely unrecognized opcode (not just an unimplemented one) still triggers unknown_op.
    //utype_vectors.hex is the instruction stream (imem). no dmem needed.
    load_hex(imem_arr, "verif/unit/core/utype_vectors.hex", 1024);

    dut.rst = 1;
    run(1);
    dut.rst = 0;
    tb.settle(); //updates sim to new inst in utype_vectors.hex

    run(1);
    CHECK_REG(1, 0x001bc000, "lui x1,0x1bc: rd = imm<<12, imm[11:0] zeroed");

    run(1);
    CHECK_REG(2, 0xaffff000, "lui x2,0xaffff: top bit set, not sign-extended");

    run(1);
    CHECK_REG(0, 0x00000000, "lui x0,0xa357: x0 stays 0");
    CHECK_EQ(dut.unknown_op, 1, "word 0x7f: unrecognized opcode, unknown_op high");

    run(1);
    CHECK_REG(1, 0x001bc000, "word 0x7f,rd=x1: x1 unchanged, proving rd_we=0");
    CHECK_EQ(dut.pc_q, 0x00000010, "word 0x7f: pc should still advance to addi (0x10)");
    CHECK_EQ(dut.unknown_op, 0, "addi x3,x0,5: unknown_op =0 ");

    //auipc: full datapath, control decode -> mux_a (alu_src_a selects pc) + mux_b (alu_src_b selects imm) -> alu (ALU_ADD) -> writeback (RESULT_ALU).
    run(1);

    run(1);
    CHECK_REG(3, 0xaffff014, "auipc x3,0xaffff: rd = pc + imm<<12 = 0x14 + 0xaffff000");

    run(1);
    CHECK_REG(0, 0x00000000, "auipc x0,0x1294: x0 stays 0");
    CHECK_EQ(dut.unknown_op, 1, "word 0x7f: unrecognized opcode, unknown_op high");

    run(1);
    CHECK_REG(1, 0x001bc000, "word 0x7f,rd=x1: x1 unchanged, proving rd_we=0");
    CHECK_EQ(dut.pc_q, 0x00000020, "word 0x7f: pc advances to addi (0x20)");
    CHECK_EQ(dut.unknown_op, 0, "addi x4,x0,5: unknown_op =0 ");

    //load: control decode -> regfile -> imm_gen -> alu (addr = rs1+imm) -> dmem -> load_ext (byte/half select + sign/zero extend) -> writeback.

    load_hex(imem_arr, "verif/unit/core/load_vectors.hex", 1024);
    load_hex(dmem_arr, "verif/unit/core/data.hex", 1024);

    dut.rst = 1;
    run(1);
    dut.rst = 0;
    tb.settle(); //fetch first load

    run(1);
    CHECK_REG(1, 0x00000004, "lb x1,4(x0): dmem[1] byte0 = 0x04");

    run(1);
    CHECK_REG(3, 0xffffffcc, "lb x3,0(x0): dmem[0] byte0 0xcc sign-extended (negative byte)");

    run(1);
    CHECK_REG(1, 0x00000009, "lh x1,5(x1): addr 9, addr[1]=0 -> low half of dmem[2] = 9");

    run(1);
    CHECK_REG(2, 0xffffabbb, "lh x2,20(x1): addr 29 low half 0xabbb, sign-extended");

    run(1);
    CHECK_REG(2, 0x00001236, "lh x2,22(x1): addr 31, addr[1]=1 -> high half 0x1236");

    run(1);
    CHECK_REG(2, 0x0000abbb, "lhu x2,20(x1): low half zero-extended");

    run(1); //.word 0x7f (unknown op): no writeback; x1 is still used with the right value in the lbu below

    run(1);
    CHECK_REG(0, 0x00000000, "lw x0,3(x1): write to x0 discarded");

    run(1);
    CHECK_REG(1, 0x0000000c, "lbu x1,-8(x1): addr 1, byte lane 1 = 0x0c zero-extended (neg offset)");

    //store: control decode -> regfile -> imm_gen -> alu (addr = rs1+imm) -> wstrb_gen (byte/half lane select) -> dmem masked write.
    //store_vectors.hex is the instruction stream (imem). data.hex (reused) preloads dmem.
    load_hex(imem_arr, "verif/unit/core/store_vectors.hex", 1024);
    load_hex(dmem_arr, "verif/unit/core/data.hex", 1024);

    dut.rst = 1;
    run(1);
    dut.rst = 0;
    tb.settle(); //fetch first store setup lw

    run(1); //lw x2,28(x0): x2 = 0x1236abbb
    run(1); //lw x4,32(x0): x4 = 0x02347399
    run(1); //lw x3,4(x0): x3 = 4

    run(1);
    CHECK_MEM(6, 0x00000004, "sw x3,23(x3): dmem[6] = x3 = 4");

    run(1);
    CHECK_MEM(6, 0x1236abbb, "sw x2,25(x0): dmem[6] fully overwritten with x2");

    run(1);
    CHECK_MEM(6, 0x123699bb, "sb x4,25(x0): addr_lo=1, lane1 = x4[7:0]=0x99");

    run(1);
    CHECK_MEM(6, 0x993699bb, "sb x4,23(x3): addr_lo=3, lane3 = x4[7:0]=0x99");

    run(1);
    CHECK_MEM(5, 0x0000abbb, "sh x2,20(x0): addr_lo=0, low half = x2[15:0]=0xabbb");

    run(1);
    CHECK_MEM(5, 0xabbbabbb, "sh x2,19(x3): addr_lo=3 -> upper half (addr_lo[0] ignored) = 0xabbb");

    return tb_report();
}
