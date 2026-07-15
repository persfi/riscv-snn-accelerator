// mux_a: ALU operand A select mux.
// Chooses the ALU's a-input between rs1_data (most instructions) and pc_q
// (auipc, which needs pc + imm rather than rs1 + imm).
//
// Interface:
//   rs1_data  [in]  - register file rs1 read data
//   pc_q      [in]  - current instruction's PC
//   alu_src_a [in]  - select: 0 = rs1_data, 1 = pc_q
//   a         [out] - selected ALU operand A

module mux_a (
    input [31:0] rs1_data,
    input [31:0] pc_q,
    input alu_src_a,
    output [31:0] a
);

    assign a = alu_src_a? pc_q : rs1_data;

endmodule
