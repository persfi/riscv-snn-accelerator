
module blink (
    input wire clk,
    output reg led0_b,
    output wire led0_g
);

    reg [25:0] cnt;
    wire clk50;
    wire pll_locked;
    always @(posedge clk50) begin
        
        cnt <= cnt+1;
        led0_b<=cnt[25];

    end

    clk_gen clk_gen(
        .clk_in(clk),
        .rst_in(1'b0),
        .clk_out(clk50),
        .locked(pll_locked)
    );

    assign led0_g = pll_locked;


endmodule
