lw x1, 4(x0) # x1 = 4
lw x2, 8(x0) # x2=9

jal x0, far_target # x0 remain 0

target2:
addi x2, x2, 5 # 1st: 9+5=14 (0xe) / 2nd: 0x1c+5=0x21 / stop the loop

jal x1, target # x1 = 0x14
addi x2, x2, 5 # skipped

target:
jal x2, target2 # x2  = 0x1c, overwrites 14

.org 0x80c # offset from the jal (0x8) is 0x804 (2052>2048): checking inst[20]

far_target:
addi x6, x0, 1 # x6 =1 , x0 = 0
lui x6, 155 # unknown_op =1 
jal x0, target2 # back to target2
