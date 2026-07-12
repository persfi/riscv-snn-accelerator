lw x1, 4(x0) # load values from data first, x1=4
lw x2, 8(x0) # load values from data first, x2=9
add x3, x1, x2  # x3 = 13
sub x4, x1, x2 # x4 =  -5 = 0xFFFFFFFB -> check that funct7[5] works
lw x5, 12(x0) # x5 = 10 (1011)
and x6, x2,x5 # x6 = 8 (1000)
srl x7, x4, x1 # shift right 4 filling zero
sra x8, x4, x1 # shift right 4 filling 4 1s
lui x6, 155 # should trigger unknown_op and x6 should remain the same
