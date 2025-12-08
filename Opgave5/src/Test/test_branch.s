.globl _start
_start:
    li t0, 5
    li t1, 5

    beq t0, t1, equal

    # hvis BEQ fejler
    li a0, 'X'
    li a7, 2
    ecall
    j end

equal:
    li a0, 'O'
    li a7, 2
    ecall

end:
    li a7, 3
    ecall
