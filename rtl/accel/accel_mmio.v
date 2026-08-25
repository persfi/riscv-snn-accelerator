/* verilator lint_off UNUSEDSIGNAL */
module accel_mmio(
    input clk,
    input rst,
    input  [7:0]  host_addr,
    input  [31:0] host_wdata,
    input         host_we,
    input [31:0] count_rd,
    input image_done,
    input rd_bank,
    output [31:0] host_rdata, 
    //count or status(things accelerator produce), mux with addr read in core dmem. Zero extend the values as core takes 32 bit read

    //output to other modules in accelerator
    output reg [4:0]  t_max,
    output reg [15:0] vth1, vth2,
    output reg [2:0]  k, 
    output reg [2:0] shift,
    output start,
    output bank_ready,
    output reg [9:0]  eva_len, evb_len

);
/* verilator lint_on UNUSEDSIGNAL */

    reg done_q;
    reg a_free_q;
    reg b_free_q;
    wire [2:0] accel_status;

    always @(posedge clk) begin
        if(rst) begin
            t_max <= 0;
            vth1 <= 0;
            vth2 <= 0;
            k <= 0;
            eva_len  <= 0;
            evb_len  <= 0;
            shift<=0;
        end 
        else if (host_we) begin
            case (host_addr)
                default: ;
                8'h10: t_max <= host_wdata[4:0];
                8'h14: vth1 <= host_wdata[15:0];
                8'h18: vth2 <= host_wdata[15:0];
                8'h1C: k <= host_wdata[2:0];
                8'h24: eva_len  <= host_wdata[9:0];
                8'h28: evb_len  <= host_wdata[9:0];
                8'h30: shift <= host_wdata[2:0];
            endcase
        end
    end

    reg rd_bank_q;
    always @(posedge clk) rd_bank_q <= rd_bank;

    //status bit 0
    always @(posedge clk) begin
        if (rst)                                
            a_free_q <= 1;
        else if (host_we && host_addr == 8'h24) //host just filled a
            a_free_q <= 0; 
        else if (rd_bank && !rd_bank_q)   
            a_free_q <= 1; // accel just moved off a(to another timestep)
    end

    //status bit 1
    always @(posedge clk) begin
        if (rst)                                
            b_free_q <= 1;
        else if (host_we && host_addr == 8'h28) //host just filled b
            b_free_q <= 0; 
        else if (!rd_bank && rd_bank_q)   
            b_free_q <= 1; // accel just moved off b(to another timestep)
    end

    //status bit 2
    always @(posedge clk) begin
        if(rst||start) begin
            done_q<=0;
        end
        else if (image_done) begin
            done_q<=1;
        end
    end

    assign host_rdata  = (host_addr==8'h2C ) ? {{29'b0}, accel_status} : 
                         (host_addr[7:6] == 2'b01) ? count_rd :
                         32'b0;
    assign start = (host_we && host_addr == 8'h20);
    assign accel_status = {done_q,b_free_q,a_free_q};
    assign bank_ready = rd_bank ? !b_free_q : !a_free_q;
    
endmodule




