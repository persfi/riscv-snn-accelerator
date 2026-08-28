`ifndef PROGRAM
  `define PROGRAM "program.hex"
`endif


/* verilator lint_off UNUSEDSIGNAL */
module dmem # (
    parameter DEPTH = 1024
)(
    input clk,
    input mem_we,
    input [3:0] wstrb, //write strobe
    input [31:0] addr, 
    input [31:0] wdata,
    output [31:0] rdata
);
/* verilator lint_on UNUSEDSIGNAL */

    localparam N = $clog2(DEPTH);
    reg [31:0] mem [0:DEPTH-1] /* verilator public_flat_rw */;

    initial begin 
        $readmemh(`PROGRAM, mem); 
        // search for it in whatever directory the simulation is run at
    end

    always @(posedge clk) begin
        if(mem_we) begin
            case (wstrb)
                default: ;
                4'b1111: mem[addr[N+1:2]] <= wdata;
                4'b0011: mem[addr[N+1:2]][15:0] <= wdata[15:0];
                4'b1100: mem[addr[N+1:2]][31:16] <= wdata[15:0];
                4'b0001: mem[addr[N+1:2]][7:0] <= wdata[7:0];
                4'b0010: mem[addr[N+1:2]][15:8] <= wdata[7:0];
                4'b0100: mem[addr[N+1:2]][23:16] <= wdata[7:0];
                4'b1000: mem[addr[N+1:2]][31:24] <= wdata[7:0];
            endcase
        end
    end

    assign rdata = mem[addr[N+1:2]];
    //each word is 4 bytes, one index of mem hold 1 word, so the index is #DEPTH addr(in btyes)/4 

endmodule

