
`ifndef W1_INIT
  `define W1_INIT "verif/vectors/snn_h128_k2_T20/w1.hex"
`endif
`ifndef W2_INIT
  `define W2_INIT "verif/vectors/snn_h128_k2_T20/w2.hex"
`endif

module weight_mem (
    input clk,
    input w1_we,
    input w2_we,
    input [14:0] weight_addr,
    input [31:0] weight_wdata,
    input layer_state,
    output [WEIGHT_WIDTH-1:0] weight_rdata
);
    /* verilator lint_off UNUSEDPARAM */
    `include "accel_defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    wire [WEIGHT_WIDTH-1:0] w1_rdata, w2_rdata;

    //run from project root for the correct init file path
    w_mem #(.DEPTH(25088), .INIT_FILE(`W1_INIT)) w1 (
        .clk(clk),
        .we(w1_we),
        .addr(weight_addr),
        .wdata(weight_wdata),
        .rdata(w1_rdata)
    );

    
    w_mem #(.DEPTH(512), .INIT_FILE(`W2_INIT)) w2 (
        .clk(clk),
        .we(w2_we),
        .addr(weight_addr[8:0]), // 2bits(3(12/4)words) + 7bits(clog2(128))
        .wdata(weight_wdata),
        .rdata(w2_rdata)
    );

    assign weight_rdata = layer_state? w2_rdata: w1_rdata;
    
endmodule
