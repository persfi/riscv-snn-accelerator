module top(
    input clk,
    input [0:0] btn,
    input [3:0] sw,
    output [3:0] led
);
    
    wire clk_sys;
    wire locked;

    reg [1:0] run_sync; //2 flops synchronizer 
    always @(posedge clk_sys) begin
        run_sync <= {run_sync[0], !btn[0] & locked}; 
        //unpressed and locked (stable) > no resetting
        //pressed or unlocked > reset
        //pressed > reset, unpressed but unlocked > reset(clk not ready)
    end
    wire rst = !run_sync[1];
    

    clk_gen clk_gen(
        .clk_in(clk),
        .rst_in(1'b0),
        .clk_out(clk_sys),
        .locked(locked)
    );

    soc soc(
        .clk(clk_sys),
        .rst(rst),
        .sw(sw),
        .pc_q(),
        .inst(),
        .unknown_op(),
        .print_sel(),
        .print_int_sel(),
        .print_int_data(),
        .exit_sel(),
        .exit_code(),
        .image_done(),
        .led_q(led)
    );

endmodule
