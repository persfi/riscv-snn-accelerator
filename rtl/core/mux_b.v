
module mux_b (
    input [31:0] imm,
    input [31:0] rs2_data,
    input alu_src_b,
    output [31:0] b
);

    assign b = alu_src_b? imm: rs2_data;

endmodule
