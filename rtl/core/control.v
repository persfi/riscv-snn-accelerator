/* verilator lint_off UNUSEDSIGNAL */
module control(
    input [6:0] op,
    //input [2:0] funct3,
    //input [6:0] funct7,
    output reg rd_we,
    output reg [2:0] imm_src,
    output reg alu_src,
    output reg [3:0] alu_ctrl,
    output reg result_src,
    output reg unknown_op
);
/* verilator lint_on UNUSEDSIGNAL */

    /* verilator lint_off UNUSEDPARAM */
    `include "defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    always @(*) begin
        unknown_op = 1;
        rd_we  = 0;
        imm_src = IMM_NONE;
        alu_src = 0;
        alu_ctrl = ALU_NONE;
        result_src = 0;

        casez (op)
            OP_LOAD: begin
                unknown_op = 0;
                rd_we = 1;
                imm_src = IMM_I;
                alu_src = 1;
                alu_ctrl = ALU_ADD;
                result_src = 1;
            end
            default ;
        endcase

    end


endmodule
