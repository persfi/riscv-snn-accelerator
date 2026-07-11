
module core (
    input clk,
    input rst,
    output [31:0] pc_q,
    output [31:0] inst
);

    wire [31:0] pc_next;
    assign pc_next = pc_q + 32'd4;

    pc pc (
        .pc_next(pc_next),
        .clk(clk),
        .rst(rst),
        .pc_q(pc_q)
    );

    imem imem (
        .addr(pc_q),
        .inst(inst)
    );

endmodule
