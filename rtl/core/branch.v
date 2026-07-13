
module branch (
    input branch_ctrl,
    input [31:0] rs1_data,
    input [31:0] rs2_data,
    input [2:0] funct3,
    output reg pc_src
);

    always @(*) begin
        pc_src = 1'b0;
        if(branch_ctrl) begin
            case (funct3)
            3'b000: pc_src = (rs1_data == rs2_data);                    
            3'b001: pc_src = (rs1_data != rs2_data);                    
            3'b100: pc_src = ($signed(rs1_data) < $signed(rs2_data));  
            3'b101: pc_src = ($signed(rs1_data) >= $signed(rs2_data)); 
            3'b110: pc_src = (rs1_data < rs2_data);                     
            3'b111: pc_src = (rs1_data >= rs2_data);                    
            default: ;
            endcase
        end
    end

endmodule
