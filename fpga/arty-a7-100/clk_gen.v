module clk_gen (
   input clk_in, 
   input rst_in, 
   output clk_out, 
   output locked
);

   // PLLE2_BASE: Base Phase Locked Loop (PLL)
   //             Artix-7
   // Xilinx HDL Language Template, version 2026.1
    wire fb;
    PLLE2_BASE #(
      .BANDWIDTH("OPTIMIZED"),  // OPTIMIZED, HIGH, LOW
      .CLKFBOUT_MULT(10),        
      .CLKFBOUT_PHASE(0.0),    
      .CLKIN1_PERIOD(10.0),      // Input clock period in ns to ps resolution (i.e. 33.333 is 30 MHz).

      //VCO = clk_in × CLKFBOUT_MULT / DIVCLK_DIVIDE (should be 800 to 1600)
      //clk_out = VCO / CLKOUT0_DIVIDE

      // CLKOUT0_DIVIDE - CLKOUT5_DIVIDE: Divide amount for each CLKOUT (1-128)
      .DIVCLK_DIVIDE(1),
      .CLKOUT0_DIVIDE(25),

      // CLKOUT0_DUTY_CYCLE - CLKOUT5_DUTY_CYCLE: Duty cycle for each CLKOUT (0.001-0.999).
      .CLKOUT0_DUTY_CYCLE(0.5),

      // CLKOUT0_PHASE - CLKOUT5_PHASE: Phase offset for each CLKOUT (-360.000-360.000).
      .CLKOUT0_PHASE(0.0),
      .STARTUP_WAIT("FALSE")    // Delay DONE until PLL Locks, ("TRUE"/"FALSE")
   )
   PLLE2_BASE_inst (

    
      // Clock Outputs: 1-bit (each) output: User configurable clock outputs
      .CLKOUT0(clk_out),   // 1-bit output: CLKOUT0

      // Feedback Clocks: 1-bit (each) output: Clock feedback ports
      .CLKFBOUT(fb), // 1-bit output: Feedback clock
      .LOCKED(locked),     // 1-bit output: LOCK
      .CLKIN1(clk_in),     // 1-bit input: Input clock
      // Control Ports: 1-bit (each) input: PLL control ports
      .PWRDWN(1'b0),     // 1-bit input: Power-down
      .RST(rst_in),           // 1-bit input: Reset
      // Feedback Clocks: 1-bit (each) input: Clock feedback ports
      .CLKFBIN(fb)    // 1-bit input: Feedback clock
   );

   // End of PLLE2_BASE_inst instantiation
					
				

endmodule
