// imm_gen: extracts and sign-extends the immediate field from a 32-bit
// RV32I instruction, encoding selected from instruction format.
//
// Interface:
//   inst    - full 32-bit instruction
//   imm_src - format select (IMM_I/IMM_S/IMM_B/IMM_U/IMM_J, see defs.vh)
//   imm     - sign-extended 32-bit immediate for the selected format

/* verilator lint_off UNUSEDSIGNAL */
module imm_gen(
    input [31:0] inst,
    input [2:0] imm_src,
    output reg [31:0] imm
);
/* verilator lint_on UNUSEDSIGNAL */

    /* verilator lint_off UNUSEDPARAM */
    `include "defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    always @(*) begin
        case (imm_src) 
            default: imm = 0;
            IMM_I: imm = {{21{inst[31]}},inst[30:20]};
            IMM_S: imm = {{21{inst[31]}},inst[30:25],inst[11:7]};
            IMM_B: imm = {{20{inst[31]}},inst[7],inst[30:25],inst[11:8],1'b0};
            IMM_U: imm = {inst[31:12],{12{1'b0}}};
            IMM_J: imm = {{12{inst[31]}},inst[19:12],inst[20],inst[30:21],1'b0};
        endcase
    end

endmodule
