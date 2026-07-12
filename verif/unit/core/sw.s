lw x2, 12(x0) # x2 = hex a
lw x3, 4(x0) # x3 = 4
sw x2, 25(x0) # dmem[6] = a = x2_data
lw x4, 24(x0) # check addr 24's data = a
sw x3, 23(x3) # dmem[6] addr 16(17) = x3_data = 4
lw x4, 24(x0) # check addr 24's data = 4
