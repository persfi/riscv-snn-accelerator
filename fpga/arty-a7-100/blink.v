
module blink (
    input wire CLK100MHZ,
    output reg led0_b
);

    reg [25:0] cnt;
    always @(posedge CLK100MHZ) begin
        
        cnt <= cnt+1;
        led0_b<=cnt[25];

    end


endmodule
