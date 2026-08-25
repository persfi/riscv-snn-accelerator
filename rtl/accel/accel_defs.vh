
localparam LANES = 4;           // weights accumulated per cycle
localparam WEIGHT_WIDTH  = LANES * 8;   // weight-memory read width
localparam V_WIDTH = LANES * 16; //each is 16 bits
localparam ACC_WIDTH = LANES * 32; //each is 32 bits

localparam H_MAX = 128; //32 words
localparam H_SHIFT_W = 5;

// acc_mem operation select
localparam [1:0] ACC_IDLE  = 2'b00;
localparam [1:0] ACC_WRITE = 2'b01;   // drain: accumulate into the group
localparam [1:0] ACC_CLEAR = 2'b10;   // fire sweep: zero the group for next timestep
localparam [1:0] ACC_CLEAR_ALL = 2'b11; //clear all during start of each image in prime state

localparam [1:0] V_IDLE  = 2'b00;
localparam [1:0] V_WRITE = 2'b01;   // drain: accumulate into the group
localparam [1:0] V_CLEAR_ALL = 2'b10; 
