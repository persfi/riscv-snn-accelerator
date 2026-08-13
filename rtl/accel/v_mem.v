// Membrane potentials for both layers in one array: 0..127 are layer 1, 128..139 are layer 2 (10 outputs padded to a LANES boundary).  
//word_cnt_q must stay inside the active layer's range (0..31 for layer 1, 0..2 for layer 2). 

module v_mem (
    input clk, 
    input layer_state, 
    input [1:0] ctrl, //clear from sequencer, enable write, idle
    input [4:0] word_cnt_q,
    input  [V_WIDTH-1:0] v_wdata,
    output reg [V_WIDTH-1:0] v_rdata
);


    /* verilator lint_off UNUSEDPARAM */
    `include "accel_defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    reg [15:0] v [0:139];
    wire [31:0] base = layer_state ? 32'd128 : 32'd0;
    integer i;

    always @(posedge clk) begin
        if(ctrl == V_CLEAR_ALL) begin
            for (i = 0; i < 140; i = i + 1) begin
                v[i] <= 0; 
            end
        end
        else if(ctrl == V_WRITE) begin
            for (i = 0; i < LANES; i = i + 1) begin
                v[base+word_cnt_q*LANES +i] <= v_wdata[i*16+:16];
            end
        end
    end
    
    integer j;
    always @(*) begin
            for (j = 0; j < LANES; j = j + 1) begin
                v_rdata[j*16+:16] = v[base + word_cnt_q*LANES + j];
            end
    end

endmodule 
