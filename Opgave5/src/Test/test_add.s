.globl _start
_start:
    # t0 = 7
    li t0, 7
    # t1 = 3
    li t1, 3

    # ADD
    add t2, t0, t1      # 10
    li a0, 48+10        # print '10'? nej – print ':' (58)
    li a7, 2
    ecall

    # SUB
    sub t3, t0, t1      # 4
    li a0, 48+4
    li a7, 2
    ecall

    # XOR (7 ^ 3 = 4)
    xor t4, t0, t1
    li a0, 48+4
    li a7, 2
    ecall

    # EXIT
    li a7, 3
    ecall
