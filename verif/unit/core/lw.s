lw x5, 0(x0)
lw x6, 4(x0)
lw x7, 5(x6)
add x8, x5, x6
lw x8, 13(x0)
# run make hex FILE=verif/unit/core/lw.s OUT=verif/unit/core/lw_vectors.hex to get respective instructions
