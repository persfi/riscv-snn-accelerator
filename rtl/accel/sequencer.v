module sequencer (
    input rst,
    input clk,
    
    //input start,
    input [4:0] t_max, //20
    input [9:0] eva_len, evb_len,
    input [3:0] spike,

    output spk1_we,
    output [6:0] spk1_addr,
    output [6:0] spk1_wr_data,
    output reg [1:0] v_ctrl,
    output reg [1:0] acc_ctrl,
    output layer_state,
    output [4:0] word_cnt, //to weight_addr
    output reg [4:0] word_cnt_q, //to acc and v read and write
    output [9:0] ev_idx,
    output [9:0] ev_idx_q,
    output rd_bank,
    output count_en,
    output count_clr, 
    output image_done
    
);

    /* verilator lint_off UNUSEDPARAM */
    `include "accel_defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    wire t_stall;
    wire ev_stall;
    
    // sequencer states, in execution order
    localparam [2:0] CLEAR  = 3'd0;  //start of image
    localparam [2:0] PRIME0 = 3'd1;  
    localparam [2:0] DRAIN0 = 3'd2;
    localparam [2:0] SWEEP0 = 3'd3;
    localparam [2:0] PRIME1 = 3'd4;
    localparam [2:0] DRAIN1 = 3'd5;
    localparam [2:0] SWEEP1 = 3'd6;

    reg [2:0] state;
    
    wire [4:0] t;
    wire [9:0] ev_len;

    wire [4:0] word_limit = layer_state ? 5'd2 : 5'd31;
    
    //time_step counter
    counter #(.W(5)) c_t (
        .rst(rst),
        .clk(clk),
        .clr(0),
        .stall(t_stall),
        .cnt_limit(t_max-1),
        .q(t)
    );

    counter #(.W(10)) c_ev (
        .rst(rst),
        .clk(clk),
        .clr(clr),
        .stall(ev_stall),
        .cnt_limit(ev_len-1),
        .q(ev_idx)
    );

    counter #(.W(5)) c_word (
        .rst(rst),
        .clk(clk),
        .clr(clr),
        .stall(word_cnt_stall),
        .cnt_limit(word_limit),
        .q(word_cnt)
    );


    always @(*) begin  
        case (state)
            CLEAR: begin //start of img
                v_ctrl = V_CLEAR_ALL;
                acc_ctrl = ACC_CLEAR_ALL;
                layer_state = 0;
                end
            PRIME0: begin
                v_ctrl = V_IDLE;
                acc_ctrl = ACC_IDLE;
                layer_state = 0;
                end
            DRAIN0: begin
                v_ctrl = V_IDLE;
                acc_ctrl = ACC_WRITE;
                layer_state = 0;
                end
            SWEEP0: begin
                 v_ctrl = (pending !=0 )? V_IDLE:V_WRITE;
                 acc_ctrl = (pending !=0 )? ACC_IDLE:ACC_CLEAR;
                 layer_state = 0;
                 end
            PRIME1: begin
                v_ctrl = V_IDLE;
                acc_ctrl = ACC_IDLE;
                layer_state = 1;
                end
            DRAIN1: begin
                v_ctrl = V_IDLE;
                acc_ctrl = ACC_WRITE;
                layer_state = 1;
                end
            SWEEP1: begin
                 v_ctrl =V_WRITE;
                 acc_ctrl = ACC_CLEAR;
                 layer_state = 1;
                end
            default: begin 
                v_ctrl = V_IDLE ; 
                acc_ctrl = ACC_IDLE;
                layer_state = 0;
            end
        endcase
    end

    wire clr =(state == CLEAR) || ((state == SWEEP0 || state == SWEEP1) && word_cnt_q == word_limit);
    wire drain_done = (state==DRAIN0 || state==DRAIN1) && ev_idx_q == ev_len-1 && word_cnt_q == word_limit;
    wire sweep0_start = drain_done && state==DRAIN0;
    wire sweep1_start = (drain_done && state==DRAIN1) || (state==PRIME1 &&spk1_wr_ptr==0 &&pending==0);
     
    always @(posedge clk) begin
        if (rst) begin
            state <= CLEAR;
            ev_idx_q <= 0;
            word_cnt_q <= 0;
        end
        else begin
            ev_idx_q <= ev_idx;
            if(word_cnt_stall==0) begin
                word_cnt_q <= word_cnt; //cnt_q updates only when word_cnt does
            end

            //state change conditions
            if(state == CLEAR) begin//clear-> prime0
                state <= PRIME0;
                ev_idx_q <= 0;
                word_cnt_q <= 0;
            end
            else if(state == PRIME0) begin //prime0 -> drain0
                state <= DRAIN0;
            end
            else if(sweep0_start) begin //drain0 -> sweep0
                state <= SWEEP0;
            end
            else if(state == SWEEP0 && word_cnt_q == word_limit &&pending==0) begin //sweep0 -> prime1
                state <= PRIME1;  
            end
            else if(state == PRIME1 && pending==0 && spk1_wr_ptr!=0) begin//prime1 -> drain1
                state <= DRAIN1; 
            end
            else if(sweep1_start) begin//drain1 -> sweep1
                state <= SWEEP1; 
            end
            else if(state == SWEEP1 && word_cnt_q == word_limit) begin //sweep1 -> prime0
                state <= PRIME0;
            end

        end
    end

    //spk1 push block
    reg [4:0] group;
    always @(posedge clk) begin
        if(rst||sweep0_start)begin
            pending   <= 0;
            spk1_wr_ptr <= 0;
        end
        else if(spike!=0 && pending ==0 && state==SWEEP0) begin
            pending <= spike;
            group <= word_cnt_q;
        end
        else if(pending !=0) begin //dont add state guard so that it drains in prime
            pending <= pending & ~(4'b1<<lane);
            spk1_wr_ptr <= spk1_wr_ptr+1;
        end
    end

    reg [3:0] pending;
    reg [6:0] spk1_wr_ptr; //potential issue
    wire word_cnt_stall;
    wire [1:0] lane;
    assign word_cnt_stall = pending != 4'b0;
    assign lane = pending[0] ? 2'd0 :
              pending[1] ? 2'd1 :
              pending[2] ? 2'd2 : 2'd3;
    assign spk1_we = pending != 4'b0;
    assign spk1_wr_data = {group,lane}; 
    assign spk1_addr = spk1_we? spk1_wr_ptr : ev_idx[6:0];
    assign ev_stall = (word_cnt != word_limit) || ((state != DRAIN0)&&(state != DRAIN1));
    assign t_stall = !(state == SWEEP1 && word_cnt_q == word_limit)  ;
    assign rd_bank = t[0];
    assign ev_len =( layer_state==0 )?( rd_bank ? evb_len : eva_len) : {3'b0, spk1_wr_ptr};
    assign count_en  = (state == SWEEP1);//block count till sweep1
    assign count_clr = (state == CLEAR);

    assign image_done = t==t_max-1 && word_cnt_q == word_limit && state == SWEEP1;

endmodule
