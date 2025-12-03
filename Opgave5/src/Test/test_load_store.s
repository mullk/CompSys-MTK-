.data
var: .word 123

.text
.globl _start
_start:
    la t0, var     # adresse &var
    lw t1, 0(t0)   # t1 = 123

    # print '1'
    li a0, '1'
    li a7, 2
    ecall

    # print '2'
    li a0, '2'
    li a7, 2
    ecall

    # print '3'
    li a0, '3'
    li a7, 2
    ecall

    # exit
    li a7, 3
    ecall
