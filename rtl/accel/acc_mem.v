
module acc_mem #(
    parameter DEPTH=128
)(
    input clk,
    input [4:0] word_cnt_q,
    input [1:0] ctrl,
    input [ACC_WIDTH-1:0] acc_wdata,
    output reg [ACC_WIDTH-1:0] acc_rdata
);

    /* verilator lint_off UNUSEDPARAM */
    `include "accel_defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    integer i;
    reg [31:0] mem [0:DEPTH-1];
    always @(posedge clk) begin

        if(ctrl == ACC_WRITE) begin
            for (i = 0; i < LANES; i = i + 1) begin
                mem[word_cnt_q*LANES+i] <= acc_wdata[i*32+:32]; 
                //drain1 cnt: 0-31, drain2 cnt:0-3
            end
        end
        else if(ctrl == ACC_CLEAR) begin
            for (i = 0; i < LANES; i = i + 1) begin
                mem[word_cnt_q*LANES+i] <= 0;
            end
        end
        
    end 

    integer j;
    always @(*) begin
        for (j = 0; j < LANES; j = j + 1) begin
            acc_rdata[j*32+:32] = mem[word_cnt_q*LANES+j];
        end
    end

endmodule
