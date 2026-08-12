//`ifndef DEFS_VH
//`define DEFS_VH
// no ifndef guard or else only the include in the first module file in core.v would be recognized

localparam [3:0] ALU_ADD  = 4'b0000;
localparam [3:0] ALU_SUB  = 4'b1000;
localparam [3:0] ALU_SLL  = 4'b0001;
localparam [3:0] ALU_SLT  = 4'b0010;
localparam [3:0] ALU_SLTU = 4'b0011;
localparam [3:0] ALU_XOR  = 4'b0100;
localparam [3:0] ALU_SRL  = 4'b0101;
localparam [3:0] ALU_SRA  = 4'b1101;
localparam [3:0] ALU_OR   = 4'b0110;
localparam [3:0] ALU_AND  = 4'b0111;
localparam [3:0] ALU_NONE = 4'b1111;

localparam [2:0] IMM_I  = 3'b000; //refer to opcode
localparam [2:0] IMM_S  = 3'b010;
localparam [2:0] IMM_B  = 3'b110;
localparam [2:0] IMM_U  = 3'b111;
localparam [2:0] IMM_J  = 3'b001;
localparam [2:0] IMM_NONE = 3'b011;

localparam [6:0] OP_LOAD = 7'b0000011;
localparam [6:0] OP_R = 7'b0110011;
localparam [6:0] OP_S = 7'b0100011;
localparam [6:0] OP_B = 7'b1100011;
localparam [6:0] OP_IMM = 7'b0010011;
localparam [6:0] OP_JAL = 7'b1101111;
localparam [6:0] OP_JALR = 7'b1100111;
localparam [6:0] OP_LUI = 7'b0110111;
localparam [6:0] OP_AUIPC = 7'b0010111;

localparam [1:0] ALU_OP_ADD = 2'b00;
localparam [1:0] ALU_OP_FUNCT = 2'b10;
localparam [1:0] ALU_OP_BRANCH = 2'b01;
localparam [1:0] ALU_OP_NONE = 2'b11;

localparam [1:0] RESULT_ALU = 2'b00;
localparam [1:0] RESULT_RDATA = 2'b01;
localparam [1:0] RESULT_PCP4 = 2'b10;
localparam [1:0] RESULT_LUI = 2'b11;


localparam [31:0] RESET_VECTOR = 32'h0000_0000;

localparam [31:0] PRINT_ADDR = 32'h1000_0000;
localparam [31:0] EXIT_ADDR  = 32'h1000_0004;

//`endif
