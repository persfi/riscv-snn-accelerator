
module ev_mem (
    input clk,
    input we,
    input wr_bank, //a=0,b=1
    input [9:0] wr_addr,
    input [9:0] wr_data,
    input rd_bank,
    input [9:0] rd_addr,
    output reg [9:0] rd_data
);

    reg [9:0] memA [0:783];
    reg [9:0] memB [0:783];

    always @(posedge clk) begin
        if(wr_bank==0 && we) memA[wr_addr] <= wr_data;
        else if(wr_bank==1 && we) memB[wr_addr] <= wr_data;
    end

    always @(*) begin
        if(rd_bank==0) rd_data = memA[rd_addr];
        else  rd_data = memB[rd_addr];
    end

endmodule
