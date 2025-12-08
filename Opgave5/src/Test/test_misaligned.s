.globl _start
_start:
    li t0, 1
    li t1, 2

    # ulovligt:
    sw t1, 2(t0)   # 2 er ikke word-alignet → skal give fejl

    li a7, 3
    ecall
