/* verilator lint_off UNUSEDSIGNAL */
module accel_mmio(
    input clk,
    input rst,
    input  [7:0]  host_addr,
    input  [31:0] host_wdata,
    input         host_we,
    input [31:0] count_rd,
    input [2:0] accel_status,
    output [31:0] host_rdata, 
    //count or status(things accelerator produce), mux with addr read in core dmem. Zero extend the values as core takes 32 bit read

    //output to other modules in accelerator
    output reg [4:0]  t_max,
    output reg [15:0] vth1, vth2,
    output reg [2:0]  k, 
    output start,
    output reg [9:0]  eva_len, evb_len
    
);
/* verilator lint_on UNUSEDSIGNAL */

    always @(posedge clk) begin
        if(rst) begin
            t_max <= 0;
            vth1 <= 0;
            vth2 <= 0;
            k <= 0;
            eva_len  <= 0;
            evb_len  <= 0;
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
            endcase
        end
    end

    assign host_rdata  = (host_addr==8'h2C ) ? {{29'b0},accel_status} : 
                         (host_addr[7:6] == 2'b01) ? count_rd :
                         32'b0;
    assign start = (host_we && host_addr == 8'h20);

endmodule




