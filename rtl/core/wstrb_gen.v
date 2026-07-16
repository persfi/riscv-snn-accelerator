// wstrb_gen: dmem write-strobe (byte-enable) generator.
// Derives which of dmem's 4 byte lanes a store should write from funct3
// (sb/sh/sw) and the low 2 address bits. Store-side counterpart to
// load_ext's read-side lane select.
//
// Interface:
//   funct3  [in]  - store width: 000=sb, 001=sh, 010=sw
//   addr_lo [in]  - addr[1:0], the byte lane within the target word
//   wstrb   [out] - one-hot/pair byte-enable mask for dmem's masked write

module wstrb_gen (
    input [2:0] funct3,
    input [1:0] addr_lo,
    output reg [3:0] wstrb
);

    always @(*) begin
        wstrb = 4'b0000;
        case (funct3)
            3'b000: wstrb[addr_lo] = 1'b1;
            3'b001: wstrb[{addr_lo[1],1'b0}+:2] = 2'b11;
            3'b010: wstrb = 4'b1111;
            default:;
        endcase
    end

endmodule
