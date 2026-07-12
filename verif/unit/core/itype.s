addi x5, x0, 1024 # if correct means it correctly excluded funct7
addi x4, x0, -5
addi x5, x5, 3 # make sure that read rs1 and write rd doesnt block each other, x5 should be 0x403
srli x6, x4, 4 # fills in 0 for shift bits proving funct7[5] selection works
srai x7, x4, 4 # fills in 1 for shift bits proving funct7[5] selection works
lui x5, 155 # should trigger unknown_op and x5 should remain the same
