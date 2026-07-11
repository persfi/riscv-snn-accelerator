
module regfile (
    input clk,
    input rd_we,
    input [4:0] rs1_addr,
    input [4:0] rs2_addr,
    input [4:0] rd_addr, //register destination (address)
    input [31:0] rd_data,
    output [31:0] rs1_data,
    output [31:0] rs2_data
);

    reg [31:0] registers [0:31] /* verilator public_flat_rd */;

    assign rs1_data = (rs1_addr==0) ? 0: registers[rs1_addr];
    assign rs2_data = (rs2_addr==0) ? 0: registers[rs2_addr];

    always @(posedge clk) begin
        if(rd_we && (rd_addr!=0)) begin
                registers[rd_addr] <= rd_data;
        end
    end

endmodule
