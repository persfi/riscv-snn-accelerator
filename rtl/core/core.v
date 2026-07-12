
module core (
    input clk,
    input rst,
    output [31:0] pc_q,
    output [31:0] inst,
    output unknown_op
);
   
    wire [31:0] pc_next; 
    wire rd_we; 
    wire mem_we;
    wire [2:0] imm_src; 
    wire alu_src;
    wire [3:0] alu_ctrl; 
    wire result_src; 
    wire [31:0] result; 
    wire [31:0] rs1_data;
    wire [31:0] rs2_data;
    wire [31:0] b;
    wire [31:0] alu_result;
    wire [31:0] dmem_rdata;
    wire [31:0] imm;
    
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

    control control (
        .op(inst[6:0]),
        .funct3(inst[14:12]),
        .funct7(inst[31:25]),
        .rd_we(rd_we),
        .imm_src(imm_src),
        .alu_src(alu_src),
        .alu_ctrl(alu_ctrl),
        .result_src(result_src),
        .mem_we(mem_we),
        .unknown_op(unknown_op)
    );

    regfile regfile (
        .clk(clk),
        .rd_we(rd_we),
        .rs1_addr(inst[19:15]),
        .rs2_addr(inst[24:20]),
        .rd_addr(inst[11:7]), 
        .rd_data(result),
        .rs1_data(rs1_data),
        .rs2_data(rs2_data)
    );

    imm_gen imm_gen (
        .inst(inst),
        .imm_src(imm_src),
        .imm(imm)
    );

    mux_b mux_b (
        .imm(imm),
        .rs2_data(rs2_data),
        .alu_src(alu_src),
        .b(b)
    );

    alu alu (
        .a(rs1_data),
        .b(b),
        .alu_ctrl(alu_ctrl),
        .result(alu_result)
    );

    dmem dmem (
        .clk(clk),
        .mem_we(mem_we),
        .wstrb(4'b1111),
        .addr(alu_result),
        .wdata(rs2_data),
        .rdata(dmem_rdata)

    );

    mux_r mux_r (
        .alu_result(alu_result),
        .addr_data(dmem_rdata),
        .result_src(result_src),
        .result(result)
    );



endmodule
