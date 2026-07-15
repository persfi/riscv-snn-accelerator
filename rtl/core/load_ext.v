
module load_ext (
    input  [31:0] rdata,     
    input  [1:0]  addr_lo,  // 0/1/2/3
    input  [2:0]  funct3,   
    output reg [31:0] load_data
);

    always @(*) begin
        load_data = rdata;
        case(funct3)
            3'b000: load_data = {{24{rdata[addr_lo*8+7]}},rdata[addr_lo*8+:8]};
            3'b001: load_data = { {16{rdata[(addr_lo[1]) * 16+15]}} , rdata[ (addr_lo[1]) * 16+:16]};
            3'b010: load_data = rdata;
            3'b100: load_data ={{24{1'b0}},rdata[addr_lo*8+:8]};
            3'b101: load_data ={ {16{1'b0}} , rdata[ (addr_lo[1]) * 16+:16]};
            default: ;
        endcase
    end

   
endmodule
