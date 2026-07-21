#include "mmio.h"

.section .text.init
.globl _start
_start:
    la sp, _stack_top
    la t0, _bss_start
    la t1, _bss_end
    clear_bss:
        bgeu t0, t1, end_clear_bss
        sw x0, 0(t0)
        addi t0, t0, 4
        j clear_bss

    end_clear_bss:

    call main
    la t2, EXIT_ADDR
    sw a0, 0(t2)
    1:  j 1b    
    



