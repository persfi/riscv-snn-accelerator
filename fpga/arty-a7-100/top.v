module top(
    input clk,
    input [0:0] btn,
    output [3:0] led

);
    
    wire clk50;
    wire locked;

    reg [1:0] run_sync; //2 flops synchronizer 
    always @(posedge clk50) begin
        run_sync <= {run_sync[0], !btn[0] & locked}; 
        //unpressed and locked (stable) > no resetting
        //pressed or unlocked > reset
        //pressed > reset, unpressed but unlocked > reset(clk not ready)
    end
    wire rst = !run_sync[1];
    

    clk_gen clk_gen(
        .clk_in(clk),
        .rst_in(1'b0),
        .clk_out(clk50),
        .locked(locked)
    );

    soc soc(
        .clk(clk50),
        .rst(rst),
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
