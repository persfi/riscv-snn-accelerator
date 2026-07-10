
/* verilator lint_off UNUSEDSIGNAL */
module imem # (
    parameter DEPTH = 1024
)(
    input [31:0] addr, 
    output [31:0] inst
);

/* verilator lint_on UNUSEDSIGNAL */
    localparam N = $clog2(DEPTH);
    reg [31:0] mem [0:DEPTH-1];

    //runs exactly once at the start of simulation, loads hex file into mem array.
    initial begin 
        $readmemh("program.hex", mem); 
        // search for it in whatever directory the simulation is run at
    end

    //each inst is 4 bytes = 1 word, one index of mem hold 1 word, so the index is #DEPTH addr(in btyes)/4 
    assign inst = mem[addr[N+1:2]];

endmodule
