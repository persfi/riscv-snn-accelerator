module soc #(
    parameter DMEM_DEPTH = 1024
)(
    input clk,
    input rst,
    output [31:0] pc_q,
    output [31:0] inst,
    output unknown_op,
    output print_sel,
    output print_int_sel,
    output [7:0] print_data,
    output [31:0] print_int_data,
    output exit_sel,
    output [31:0] exit_code,
    output  image_done
);

    localparam [31:0] DMEM_SIZE_BYTES = DMEM_DEPTH * 4;

    /* verilator lint_off UNUSEDPARAM */
    `include "defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    wire [31:0] core_d_addr;
    wire [31:0] core_d_wdata;
    wire [31:0] core_d_rdata;
    wire [31:0] dmem_rdata;
    wire [31:0] accel_rdata;
    wire core_d_we;
    wire [3:0] core_d_wstrb;
    wire dmem_sel;
    wire accel_sel;
    

    assign exit_sel = (core_d_addr == EXIT_ADDR) && core_d_we;
    assign exit_code = core_d_wdata;
    assign print_sel = (core_d_addr == PRINT_ADDR) && core_d_we;
    assign print_int_sel = (core_d_addr == PRINT_INT_ADDR) && core_d_we;
    assign print_data = print_sel?core_d_wdata[7:0]:8'b0;
    assign print_int_data = print_int_sel?core_d_wdata[31:0]:32'b0;
    assign dmem_sel = core_d_addr < DMEM_SIZE_BYTES;
    assign core_d_rdata = dmem_sel ? dmem_rdata : 
                    accel_sel? accel_rdata: 32'b0;
    assign accel_sel = core_d_addr[31:28] == 4'h2;

    core core (
        .clk(clk),
        .rst(rst),
        .pc_q(pc_q),
        .inst(inst),
        .unknown_op(unknown_op),

        .d_addr(core_d_addr),
        .d_wdata(core_d_wdata),
        .d_we(core_d_we),
        .d_wstrb(core_d_wstrb),
        .d_rdata(core_d_rdata)
    );

    accel accel (
        .clk(clk),
        .rst(rst),
        .host_addr(core_d_addr),
        .host_wdata(core_d_wdata),
        .host_we(core_d_we && accel_sel),
        .image_done(image_done),
        .host_rdata(accel_rdata)
    );

    dmem #(
        .DEPTH(DMEM_DEPTH)
    ) dmem (
        .clk(clk),
        .mem_we(core_d_we && dmem_sel),
        .wstrb(core_d_wstrb),
        .addr(core_d_addr),
        .wdata(core_d_wdata ),
        .rdata(dmem_rdata)
    );

endmodule
