
/* verilator lint_off UNUSEDSIGNAL */
module dmem # (
    parameter DEPTH = 1024
)(
    input [31:0] addr, 
    output [31:0] data // becomes rdata when write port lands
);

/* verilator lint_on UNUSEDSIGNAL */

    localparam N = $clog2(DEPTH);
    reg [31:0] mem [0:DEPTH-1] /* verilator public_flat_rw */;

    //runs exactly once at the start of simulation, loads hex file into mem array.
    initial begin 
        $readmemh("data.hex", mem); 
        // search for it in whatever directory the simulation is run at
    end

    //each word is 4 bytes, one index of mem hold 1 word, so the index is #DEPTH addr(in btyes)/4 
    assign data = mem[addr[N+1:2]];

endmodule
