addi x1, x0, 5
addi x2, x0, 5

add x5, x1,x2 # should not trigger beq and pc_q, check pc_q

beq  x1, x2, target # taken          
addi x3, x0, 1 # must be skipped
target:
addi x3, x0, 2 # x3=2


bne x1, x2, target2 # not taken
addi x3, x0, 3 # x3=3
target2:
nop

target3:
addi x3, x3, 2 # x3 = 5 -> 7
bge x2,x3,target3 # not taken from the second time on / test backwards jump


addi x4, x0, -10
blt x4, x3, target4 # take

target5:
addi x5, x3, 2 # x5 should not be 2 (not taken)

target4:
bltu x4, x3, target5 # should not take
