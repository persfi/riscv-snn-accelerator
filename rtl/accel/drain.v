
module drain (
    input [WEIGHT_WIDTH-1:0] weight_rdata,
    input [ACC_WIDTH-1:0] acc_rdata,
    output reg [ACC_WIDTH-1:0] acc_wdata
);

    /* verilator lint_off UNUSEDPARAM */
    `include "accel_defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    integer i;
    reg [31:0] weight_extend;
    always @(*) begin
        for(i=0;i<LANES;i=i+1) begin
            weight_extend = {{24{weight_rdata[i*8+7]}},weight_rdata[i*8+:8]};
            acc_wdata[i*32+:32] = acc_rdata[i*32+:32] + weight_extend;
        end
    end

endmodule
