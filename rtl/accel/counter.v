
module counter #(parameter W = 5) (
    input rst,
    input clk,
    input stall,
    input [W-1:0] cnt_limit,
    output reg [W-1:0] q
);

    always @(posedge clk) begin
        if(rst) q<=0;
        else if(stall) q<=q;
        else if(q==cnt_limit) q<=0;
        else q<=q+1;
    end

endmodule
