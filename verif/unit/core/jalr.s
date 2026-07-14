addi x1, x0, 5
jalr x2, 12(x1) # pc should jump to 5+12, truncate to 16 due to the 0 last bit/check x2 = 8
addi x3, x0, 3 # 1: should be skipped/2: should check that x3=3
jalr x3, 17(x3) # check that x3 should be 16 and should jump to 20 (make sure rs1_data doesnt get jammed with write rd)
jalr x0, 0(x2) # 8+0=8, should jump to addi of x3 / check x0 remain 0 not 20
addi x3, x3, -16 
jalr x4, -8(x2) # should jump to first inst (0x0)
