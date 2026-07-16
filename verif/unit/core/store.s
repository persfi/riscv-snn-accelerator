lw x2, 28(x0) # x2 = hex 1236abbb
lw x4, 32(x0) # x4 = 02347399
lw x3, 4(x0) # x3 = 4
sw x3, 23(x3) # dmem[6]= x3_data = 4 -> check mem
sw x2, 25(x0) # dmem[6]  = x2_data 
sb x4, 25(x0) # dmem[6] = 0x123699bb
sb x4, 23(x3) # dmem[6] = 0x993699bb
sh x2, 20(x0) # dmem[5] = 0x0000abbb
sh x2, 19(x3) # dmem[5] = 0xabbbabbb


