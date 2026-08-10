module addr_gen(
    input layer_state,
    input [4:0] word_cnt,
    input [9:0] ev_rd_data,
    input [6:0] spk1_rd_data,
    output reg [14:0] weight_addr
);

    always @(*) begin
        if(layer_state==0) begin
            weight_addr = {ev_rd_data,word_cnt};
        end
        else begin
            weight_addr = {{6{1'b0}},spk1_rd_data,word_cnt[1:0]};
        end
    end

endmodule
