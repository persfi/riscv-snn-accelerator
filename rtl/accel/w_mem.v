module w_mem #(
    parameter DEPTH = 25088,
    localparam AW = $clog2(DEPTH) //rounds to 15/9
)(
    input clk,
    input we,
    input [AW-1:0] addr,
    input [31:0] wdata,
    output reg [WEIGHT_WIDTH-1:0] rdata
);
    /* verilator lint_off UNUSEDPARAM */
    `include "accel_defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    reg [WEIGHT_WIDTH-1:0] mem [0:DEPTH-1];  
    always @(posedge clk) begin
        if(we) begin
            mem[addr] <= wdata;
        end
        rdata <= mem[addr];
    end

endmodule
