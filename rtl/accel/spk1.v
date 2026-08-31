
//idx event queue for high neurons
module spk1 (
    input clk,
    input we,
    input [6:0] addr,
    input [6:0] wr_data,
    output [6:0] rd_data
);

    reg [6:0] mem [0:127] /* verilator public_flat_rd */; //max 128 -> 7 bits

    always @(posedge clk) begin
        if(we) begin
            mem[addr] <= wr_data;
        end
    end

    assign rd_data = mem[addr];

endmodule
