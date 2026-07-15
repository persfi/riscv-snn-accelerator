lui x1, 0x1bc
lui x2, 0xaffff
lui x0, 0xa357 # x0 should be 0
.word  0x000000ff # unrecognizable opcode (0x7f) with rd=x1 (nonzero), so an erroneous write would be visible
addi x3, x0, 5
