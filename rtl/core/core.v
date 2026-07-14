
module core (
    input clk,
    input rst,
    output [31:0] pc_q,
    output [31:0] inst,
    output unknown_op
);
   
    wire [31:0] pc_plus4; 
    wire [31:0] pc_next; 
    wire rd_we; 
    wire mem_we;
    wire [2:0] imm_src; 
    wire alu_src;
    wire [3:0] alu_ctrl; 
    wire [1:0] result_src; 
    wire [31:0] result; 
    wire [31:0] rs1_data;
    wire [31:0] rs2_data;
    wire [31:0] b;
    wire [31:0] alu_result;
    wire [31:0] dmem_rdata;
    wire [31:0] imm;
    wire [31:0] pc_target;
    wire [31:0] pc_target_sum;
    wire pc_target_src;
    wire pc_src;
    wire branch_ctrl;
    wire jump;
    wire branch_taken;
    
    assign pc_plus4 = pc_q + 32'd4;
    assign pc_target_sum = (pc_target_src ? rs1_data : pc_q) + imm;
    assign pc_target = pc_target_src ? {pc_target_sum[31:1], 1'b0}: pc_target_sum;
    assign pc_src = branch_taken | jump;

    pc pc (
        .pc_next(pc_next),
        .clk(clk),
        .rst(rst),
        .pc_q(pc_q)
    );

    mux_pc mux_pc (
        .pc_src(pc_src),
        .pc_plus4(pc_plus4),
        .pc_target(pc_target),
        .pc_next(pc_next)
    );

    branch branch (
        .branch_ctrl(branch_ctrl),
        .rs1_data(rs1_data),
        .rs2_data(rs2_data),
        .funct3(inst[14:12]),
        .branch_taken(branch_taken)
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
        .branch_ctrl(branch_ctrl),
        .jump(jump),
        .pc_target_src(pc_target_src),
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
        .dmem_rdata(dmem_rdata),
        .pc_plus4(pc_plus4),
        .result_src(result_src),
        .result(result)
    );



endmodule
