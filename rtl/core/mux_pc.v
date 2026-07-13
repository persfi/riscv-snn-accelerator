
module mux_pc(
    input pc_src,
    input [31:0] pc_plus4,
    input [31:0] pc_target,
    output [31:0] pc_next
);

    assign pc_next = pc_src? pc_target: pc_plus4;

endmodule
