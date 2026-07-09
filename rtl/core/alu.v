
module alu(
    input [31:0] a,
    input [31:0] b,
    input [3:0] alu_ctrl,
    output reg [31:0] result
);
    

    /* verilator lint_off UNUSEDPARAM */
    `include "defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    always @(*) begin
        case (alu_ctrl) // '=' for combinational logic
            ALU_ADD: result = a + b; 
            ALU_SUB: result = a - b;
            ALU_XOR: result = a ^ b;
            ALU_OR: result = a | b;
            ALU_AND: result = a & b;
            ALU_SLL: result = a << b[4:0];
            ALU_SRL: result = a >> b[4:0];
            ALU_SRA: result = $unsigned($signed(a) >>> b[4:0]);
            ALU_SLT: result = $signed(a) < $signed(b) ? 1 : 0;
            ALU_SLTU: result = a < b ? 1 : 0;
            default: result = 0;
        endcase
    end



endmodule
