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
    output reg mem_we,
    //output reg [3:0] wstrb,
    output reg unknown_op
);
/* verilator lint_on UNUSEDSIGNAL */

    /* verilator lint_off UNUSEDPARAM */
    `include "defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    always @(*) begin     
        rd_we  = 0;
        imm_src = IMM_NONE;
        alu_src = 0;
        alu_ctrl = ALU_NONE;
        result_src = 0;
        mem_we = 0;
        unknown_op = 1;

        casez (op)
            OP_LOAD: begin
                rd_we = 1;
                imm_src = IMM_I;
                alu_src = 1;
                alu_ctrl = ALU_ADD;
                result_src = 1;
                mem_we = 0;
                unknown_op = 0;
            end
            OP_S: begin
                rd_we = 0;
                imm_src = IMM_S;
                alu_src = 1;
                alu_ctrl = ALU_ADD;
                result_src = 0; //rd_we=0 so doesn't matter which one got chosen
                mem_we = 1;
                unknown_op = 0;
            end
            default ;
        endcase

    end


endmodule
