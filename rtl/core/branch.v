
module branch (
    input branch_ctrl,
    input [31:0] rs1_data,
    input [31:0] rs2_data,
    input [2:0] funct3,
    output reg branch_taken
);

    always @(*) begin
        branch_taken = 1'b0;
        if(branch_ctrl) begin
            case (funct3)
            3'b000: branch_taken = (rs1_data == rs2_data);                    
            3'b001: branch_taken = (rs1_data != rs2_data);                    
            3'b100: branch_taken = ($signed(rs1_data) < $signed(rs2_data));  
            3'b101: branch_taken = ($signed(rs1_data) >= $signed(rs2_data)); 
            3'b110: branch_taken = (rs1_data < rs2_data);                     
            3'b111: branch_taken = (rs1_data >= rs2_data);                    
            default: ;
            endcase
        end
    end

endmodule
