lb x1, 4(x0) # x1 = 4
lb x3, 0(x0) # x1 = ffffffcc
lh x1, 5(x1) # x1 = 9
lh x2, 20(x1) # 29 -> 28 lower half. x2 = ffffabbb
lh x2, 22(x1) # 31 -> 30 upper half. x2 = 00001236
lhu x2, 20(x1) # x2 = 0000abbb
.word  0x000000ff # unrecognizable opcode (0x7f) with rd=x1, check rd is still 9
lw x0, 3(x1) # x0 stays 0
lbu x1, -8(x1) # addr 1 -> takes 0000000c = x1

