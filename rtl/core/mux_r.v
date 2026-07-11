module mux_r (
    input [31:0] alu_result,
    input [31:0] addr_data,
    input result_src,
    output [31:0] result
);

    assign result = result_src? addr_data : alu_result;

endmodule
