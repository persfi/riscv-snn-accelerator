# I type: imm is the expected result
addi x5, x0, -2048 # min imm
addi x5, x0, 2047 # max imm
addi x5, x0, 3 # positive imm
addi x5 ,x0, -3 # negative imm

# S type: imm is the expected result
sw x10, -2048(x5) # min imm 
sw x10, 2047(x5) # max imm
sw x10, 7(x5) # positive imm
sw x10, -7(x5) # negative imm

# B type
back:
beq x5, x10, fwd # positive offset = 4
fwd:
beq x5, x10, back # negative offset = -4

# U type: result = 0xabcde/0xabcde000 ; 0x800000/0x80000000
# add 3 zeros in hex because 8^3 = 2^12
lui x5, 0xabcde # handles 20 bits correctly
lui x5, 0x80000 # top bit 1 handles correctly

# J type
back2:
jal x5, fwd2 # positive offset = 4
fwd2:
jal x5, back2 # negative offset = -4

# J type, wide: gap forced past 4096 bytes so the offset sets imm[19:12] 
# couldn't test in core tb since it's only 1024 words (4096 bytes) deep
back3:
jal x5, fwd3 
.org 0x1038
fwd3:
jal x5, back3

