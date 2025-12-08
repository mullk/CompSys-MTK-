.globl _start
_start:
    li t0, 6
    li t1, 3

    mul t2, t0, t1  # 18
    div t3, t0, t1  # 2

    li a0, 'O'
    li a7, 2
    ecall

    li a7, 3
    ecall
