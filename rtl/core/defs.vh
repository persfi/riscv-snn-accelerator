`ifndef DEFS_VH
`define DEFS_VH

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

localparam [2:0] IMM_I  = 3'b000; //refer to opcode
localparam [2:0] IMM_S  = 3'b010;
localparam [2:0] IMM_B  = 3'b110;
localparam [2:0] IMM_U  = 3'b111;
localparam [2:0] IMM_J  = 3'b001;

localparam [31:0] RESET_VECTOR = 32'h0000_0000;

`endif
