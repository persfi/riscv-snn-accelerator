/* verilator lint_off UNUSEDSIGNAL */
module control(
    input [6:0] op,
    input [2:0] funct3,
    input [6:0] funct7,
    output reg rd_we,
    output reg [2:0] imm_src,
    output reg alu_src,
    output reg [3:0] alu_ctrl, 
    output reg [1:0] result_src,
    output reg mem_we,
    output reg branch_ctrl,
    output reg jump,
    output reg pc_target_src,
    //output reg [3:0] wstrb,
    output reg unknown_op
);
/* verilator lint_on UNUSEDSIGNAL */

    /* verilator lint_off UNUSEDPARAM */
    `include "defs.vh"
    /* verilator lint_on UNUSEDPARAM */
    reg [1:0] alu_op;

    //main decoder
    always @(*) begin     
        rd_we  = 0;
        imm_src = IMM_NONE;
        alu_src = 0;
        alu_op = ALU_OP_NONE;
        result_src = RESULT_NONE;
        mem_we = 0;
        jump=0;
        pc_target_src=0;
        unknown_op = 1;
        case (op)
            OP_LOAD: begin
                rd_we = 1;
                imm_src = IMM_I;
                alu_src = 1;
                alu_op = ALU_OP_ADD;
                result_src = RESULT_RDATA;
                mem_we = 0;
                jump=0;
                pc_target_src=0;
                unknown_op = 0;
            end
            OP_S: begin
                rd_we = 0;
                imm_src = IMM_S;
                alu_src = 1;
                alu_op = ALU_OP_ADD;
                result_src = RESULT_NONE; //rd_we=0 so doesn't matter which one got chosen
                mem_we = 1;
                jump=0;
                pc_target_src=0;
                unknown_op = 0;
            end
            OP_R: begin
                rd_we = 1;
                imm_src = IMM_NONE;
                alu_src = 0;
                alu_op = ALU_OP_FUNCT;
                result_src = RESULT_ALU; 
                mem_we = 0;
                jump=0;
                pc_target_src=0;
                unknown_op = 0;
            end
            OP_IMM: begin
                rd_we = 1;
                imm_src = IMM_I;
                alu_src = 1;
                alu_op = ALU_OP_FUNCT;
                result_src = RESULT_ALU; 
                mem_we = 0;
                jump=0;
                pc_target_src=0;
                unknown_op = 0;
            end
            OP_B: begin
                rd_we = 0;
                imm_src = IMM_B;
                alu_src = 0;
                alu_op = ALU_OP_BRANCH;
                result_src = RESULT_NONE; 
                mem_we = 0;
                jump=0;
                pc_target_src=0;
                unknown_op = 0;
            end
            OP_JAL: begin
                rd_we = 1;
                imm_src = IMM_J;
                alu_src = 0; // won't use
                alu_op = ALU_OP_NONE;
                result_src = RESULT_PCP4; 
                mem_we = 0;
                jump=1;
                pc_target_src=0;
                unknown_op = 0;
            end
            OP_JALR: begin
                rd_we = 1;
                imm_src = IMM_I;
                alu_src = 0; // won't use
                alu_op = ALU_OP_NONE;
                result_src = RESULT_PCP4; 
                mem_we = 0;
                jump=1;
                pc_target_src=1;
                unknown_op = 0;
            end
            default ;
        endcase
    end

    reg lbit;
    //alu decoder
    always @(*) begin

        branch_ctrl = 0;
        alu_ctrl = ALU_NONE;

        if((op == OP_IMM) && (funct3 != 3'b101)) begin
            lbit = 1'b0;
        end
        else begin
            lbit = funct7[5];
        end

        case (alu_op)
            ALU_OP_ADD: alu_ctrl = ALU_ADD;
            ALU_OP_FUNCT: alu_ctrl = {lbit, funct3};
            ALU_OP_BRANCH: branch_ctrl = 1;
            default: ;
        endcase
    end

endmodule
