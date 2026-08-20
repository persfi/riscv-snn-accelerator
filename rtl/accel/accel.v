/* verilator lint_off UNUSEDSIGNAL */
module accel (
    input clk,
    input rst,
    input [31:0] host_addr,
    input  [31:0] host_wdata,
    input  host_we,
    output image_done,
    output [31:0] host_rdata
);
/* verilator lint_on UNUSEDSIGNAL */

    /* verilator lint_off UNUSEDPARAM */
    `include "accel_defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    wire rd_bank;
    wire [9:0] ev_rd_data;
    wire [4:0] word_cnt_q;
    wire [4:0] word_cnt;
    wire layer_state;

    //for tb
    wire eva_sel= (host_addr[31:12]==20'h20001); 
    wire evb_sel= (host_addr[31:12]== 20'h20002);
    wire ev_we, ev_wr_bank;
    wire [9:0] ev_wr_addr, ev_wr_data;
    assign ev_we = host_we && (eva_sel || evb_sel);
    assign ev_wr_bank= evb_sel;
    assign ev_wr_addr=host_addr[11:2]; // divide by four for mem index
    assign ev_wr_data= host_wdata[9:0]; 

    wire [15:0] v_th;
    wire [1:0] acc_ctrl;
    wire [1:0] v_ctrl;
    wire [ACC_WIDTH-1:0] acc_wdata;
    wire [ACC_WIDTH-1:0] acc_rdata;
    wire [V_WIDTH-1:0] v;
    wire [V_WIDTH-1:0] v_res;
    wire [9:0] ev_idx;
    //wire [9:0] ev_idx_q;
    wire [3:0] spike;
    assign v_th = layer_state? vth2:vth1;
    wire count_en, count_clr;
    wire [4:0]  t_max;
    wire [2:0] k;
    wire [9:0]  eva_len;
    wire [9:0]  evb_len;
    wire [15:0] vth1;
    wire [15:0] vth2;
    wire start;
    
    wire bank_ready;
    wire spk1_we;
    wire [6:0] spk1_addr;
    wire [6:0] spk1_wr_data;
    wire [6:0] spk1_rd_data;
    wire [31:0] count_rd_data;
    
    wire w1_sel = (host_addr[31:16] ==  16'h2002 || host_addr[31:16] ==  16'h2003);
    wire w2_sel = (host_addr[31:16] ==  16'h2004);
    wire w1_we= host_we && w1_sel;
    wire w2_we = host_we && w2_sel;
    wire [14:0] gen_weight_addr; //from addr gen
    wire [14:0] weight_addr = (w1_we || w2_we) ? host_addr[16:2] : gen_weight_addr;;
    wire [WEIGHT_WIDTH-1:0] weight_rdata; //from weight mem

    ev_mem ev_mem (
        .clk(clk),
        .we(ev_we),
        .wr_bank(ev_wr_bank),
        .wr_addr(ev_wr_addr),
        .wr_data(ev_wr_data),
        .rd_bank(rd_bank),
        .rd_addr(ev_idx),
        .rd_data(ev_rd_data)
    );

    acc_mem acc_mem (
        .clk(clk),
        .word_cnt_q(word_cnt_q),
        .ctrl(acc_ctrl),
        .acc_wdata(acc_wdata),
        .acc_rdata(acc_rdata)
    );

    drain drain (
        .weight_rdata(weight_rdata),
        .acc_rdata(acc_rdata),
        .acc_wdata(acc_wdata)
    );

    weight_mem weight_mem (
        .clk(clk),
        .w1_we(w1_we),
        .w2_we(w2_we),
        .weight_addr(weight_addr),
        .weight_wdata(host_wdata),
        .layer_state(layer_state),
        .weight_rdata(weight_rdata) 
    );

    addr_gen addr_gen (
        .layer_state(layer_state),
        .word_cnt(word_cnt),
        .ev_rd_data(ev_rd_data),
        .spk1_rd_data(spk1_rd_data),
        .weight_addr(gen_weight_addr)
    );

    spk1 spk1 (
        .clk(clk),
        .we(spk1_we),
        .addr(spk1_addr),
        .wr_data(spk1_wr_data),
        .rd_data(spk1_rd_data)
    );


    v_mem v_mem (
        .clk(clk),
        .layer_state(layer_state),
        .ctrl(v_ctrl),
        .word_cnt_q(word_cnt_q),
        .v_wdata(v_res),
        .v_rdata(v)
    );
    /* verilator lint_off PINCONNECTEMPTY  */
     
    lif_unit lif_unit (
        .k(k),
        .v_th(v_th),
        .v(v),
        .acc(acc_rdata),
        .v_res(v_res),
        .spike(spike)
    );

    sequencer sequencer (
        .rst(rst),
        .clk(clk),
        .start(start),
        .bank_ready(bank_ready),
        .t_max(t_max),
        .eva_len(eva_len),
        .evb_len(evb_len),
        .spike(spike),
        .spk1_we(spk1_we),
        .spk1_addr(spk1_addr),
        .spk1_wr_data(spk1_wr_data),
        .v_ctrl(v_ctrl),
        .acc_ctrl(acc_ctrl),
        .layer_state(layer_state),
        .word_cnt(word_cnt),
        .word_cnt_q(word_cnt_q),
        .ev_idx(ev_idx),
        .rd_bank(rd_bank),
        .count_en(count_en),
        .count_clr(count_clr),
        .image_done(image_done)
    );

    wire accel_mmio_we = host_we && (host_addr[31:12] == 20'h20000);
    //not ev banks or weight mem

    accel_mmio accel_mmio(
        .clk(clk),
        .rst(rst),
        .host_addr(host_addr[7:0]),
        .host_wdata(host_wdata),
        .host_we(accel_mmio_we),
        .count_rd(count_rd_data),
        .image_done(image_done),
        .rd_bank(rd_bank),
        .host_rdata(host_rdata),
        .t_max(t_max),
        .vth1(vth1),
        .vth2(vth2),
        .k(k),
        .start(start),
        .bank_ready(bank_ready),
        .eva_len(eva_len),
        .evb_len(evb_len)
    );

    count count(
        .clk(clk),
        .count_en(count_en),
        .count_clr(count_clr),
        .word_cnt_q(word_cnt_q),
        .spike(spike),
        .rd_idx(host_addr[3:2]),
        .rd_data(count_rd_data)
    );

    /* verilator lint_on PINCONNECTEMPTY  */
endmodule
