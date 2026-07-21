#define PRINT_ADDR 0x10000000
#define EXIT_ADDR 0x10000004

addi x1, x0, 0x123
addi x2, x0, 0x0
li x3, PRINT_ADDR
li x4, EXIT_ADDR
sw x1, 0(x3)
sw x2, 0(x4)



