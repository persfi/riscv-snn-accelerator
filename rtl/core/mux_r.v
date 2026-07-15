module mux_r (
    input [31:0] alu_result,
    input [31:0] dmem_rdata,
    input [31:0] pc_plus4,
    input [31:0] imm,
    input [1:0] result_src,
    output reg [31:0] result
);

    /* verilator lint_off UNUSEDPARAM */
    `include "defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    always @(*) begin
        result = 0;
        case (result_src) 
            RESULT_ALU: result = alu_result;
            RESULT_RDATA: result = dmem_rdata;
            RESULT_PCP4: result = pc_plus4;
            RESULT_U: result = imm;
            default: ;
        endcase
    end

endmodule
