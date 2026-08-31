module count (
    input clk,
    input count_en,
    input count_clr,
    input [4:0] word_cnt_q,
    input [3:0] spike,
    input [1:0] rd_idx,
    output reg [31:0] rd_data
);

    /* verilator lint_off UNUSEDPARAM */
    `include "accel_defs.vh"
    /* verilator lint_on UNUSEDPARAM */


    reg [7:0] mem [0:11] /* verilator public_flat_rd */; //match count.hex

    integer i;
    always @(posedge clk) begin
        if(count_clr) begin
            for(i=0;i<12;i=i+1) begin
                mem[i] <= 0;
            end
        end
        else if(count_en) begin
            for(i=0;i<LANES;i=i+1) begin
                if(spike[i]==1 && (word_cnt_q*LANES+i < 10)) 
                    mem[word_cnt_q*LANES+i] <= mem[word_cnt_q*LANES+i]+1;
            end
        end
    end

    integer j;
    always @(*) begin
        for (j = 0; j < LANES; j = j + 1)
            rd_data[j*8 +: 8] = mem[rd_idx*LANES + j];
    end


endmodule
