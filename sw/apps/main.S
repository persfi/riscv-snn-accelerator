#include "mmio.h"

.globl main

main:
    li a0, 38
    la t0, PRINT_ADDR
    sw a0, 0(t0)
    ret
