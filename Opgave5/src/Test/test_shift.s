.globl _start
_start:
    li t0, 8     # 00001000

    slli t1, t0, 1   # 00010000 = 16
    srli t2, t1, 2   # 00000100 = 4
    srai t3, t2, 1   # 00000010 = 2

    li a0, 48+2
    li a7, 2
    ecall

    li a7, 3
    ecall
