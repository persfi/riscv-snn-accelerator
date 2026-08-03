module lif_unit (
    input [2:0] k,
    input signed [15:0] v_th,
    input signed [V_WIDTH-1:0] v,
    input signed [ACC_WIDTH-1:0] acc,
    output reg signed [V_WIDTH-1:0] v_res,
    output reg [LANES-1:0] spike
);

    /* verilator lint_off UNUSEDPARAM */
    `include "accel_defs.vh"
    /* verilator lint_on UNUSEDPARAM */

    //leak
    integer i;
    reg signed [15:0] v_temp;
    reg signed [31:0] acc_temp1, acc_temp2;
    always @(*) begin
        for (i = 0; i < LANES; i = i + 1) begin

            //leak
            v_temp = $signed(v[i*16 +: 16]) - ($signed(v[i*16 +: 16]) >>> k);
            acc_temp1 = ($signed({{16{v_temp[15]}}, v_temp})) + ($signed(acc[i*32 +: 32]));

            //integrate+fire
            if(acc_temp1 > $signed({{16{v_th[15]}}, v_th})) begin
                acc_temp2 = acc_temp1 - ($signed({{16{v_th[15]}}, v_th}));
                spike[i] = 1;
            end
            else begin
                acc_temp2 = acc_temp1;
                spike[i] = 0;
            end 

            //v_res writeback
            if (acc_temp2 > 32'sd32767) //signed decimal
                v_res[i*16 +: 16] = 16'sh7fff; //signed hex
            else if (acc_temp2 < -32'sd32768)
                v_res[i*16 +: 16] = 16'sh8000;
            else
                v_res[i*16 +: 16] = acc_temp2[15:0];
        end
    end

endmodule
