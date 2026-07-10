// pc: program counter register.
// Holds the address of the current instruction. On the rising clk edge,
// registers pc_next (the already-muxed next-PC value, e.g. pc+4 or a
// branch/jump target) unless rst is asserted, in which case it loads
// RESET_VECTOR instead.
//
// Interface:
//   pc_next [in]  - next PC value
//   clk     [in]  - system clock
//   rst     [in]  - active-high synchronous reset
//   pc_q    [out] - registered current PC value

module pc(
    input [31:0] pc_next, //already computed pc_addr from previous mux
    input clk,
    input rst,
    output reg [31:0] pc_q //q is usually the output of a flip-flop
);

    /* verilator lint_off UNUSEDPARAM */
  `include "defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    always @(posedge clk) begin
        if(rst) begin
            pc_q <= RESET_VECTOR;
        end
        else begin
            pc_q <= pc_next;
        end
    end

endmodule
