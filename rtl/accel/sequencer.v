module sequencer (
    input rst,
    input clk,
    
    //input start,
    input [4:0] t_max, //20
    input [9:0] ev_len,

    output reg [1:0] v_ctrl,
    output reg [1:0] acc_ctrl,
    output layer_state,
    output [4:0] word_cnt, //to weight_addr
    output reg [4:0] word_cnt_q, //to acc and v read and write
    output [9:0] ev_idx,
    output [9:0] ev_idx_q,
    output rd_bank,
    output image_done
);

    /* verilator lint_off UNUSEDPARAM */
    `include "accel_defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    wire t_stall;
    wire ev_stall;
    reg [1:0] state;
    
    wire [4:0] t;
    
    //time_step counter
    counter #(.W(5)) c_t (
        .rst(rst),
        .clk(clk),
        .stall(t_stall),
        .cnt_limit(t_max-1),
        .q(t)
    );

    assign rd_bank = t[0];

    counter #(.W(10)) c_ev (
        .rst(rst),
        .clk(clk),
        .stall(ev_stall),
        .cnt_limit(ev_len-1),
        .q(ev_idx)
    );

    counter #(.W(5)) c_word (
        .rst(rst),
        .clk(clk),
        .stall(0), //no stall drain
        .cnt_limit(31),
        .q(word_cnt)
    );


    always @(*) begin  
        case (state)
            0: begin 
                v_ctrl = V_IDLE;  
                acc_ctrl = ACC_WRITE;
                layer_state = state[0];
                end
            1: begin 
                 v_ctrl = V_WRITE;
                 acc_ctrl = ACC_CLEAR;
                 layer_state = state[0];
                 end
            2: begin
                v_ctrl = V_IDLE; 
                acc_ctrl = ACC_WRITE;
                layer_state = state[0];
                end
            default: begin 
                v_ctrl = V_IDLE ; 
                acc_ctrl = ACC_IDLE;
                layer_state = 0;
            end
        endcase
    end

    always @(posedge clk) begin
        if (rst) begin
            state <= 2'b11;
            ev_idx_q <= 0;
            word_cnt_q <= 0;
         
        end
        else begin
            word_cnt_q <= word_cnt;
            ev_idx_q <= ev_idx;

            if      (state == 0 && ev_idx_q == ev_len-1 && word_cnt_q == 31) state <= 1;
            else if (state == 1 && word_cnt_q == 31)                       state <= 0;
            else if(state == 2'b11)
            state <=0;
            
        end


    end

    assign ev_stall = (word_cnt != 31) || (state != 0);
    assign t_stall = !(state == 1 && word_cnt_q == 31)  ;
    assign image_done = t==t_max && word_cnt_q == 31 && state == 3;

endmodule
