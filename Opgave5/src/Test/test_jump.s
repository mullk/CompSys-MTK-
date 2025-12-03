.globl _start
_start:
    jal ra, func

    # Hvis JAL fejler:
    li a0, 'E'
    li a7, 2
    ecall
    j exitprog

func:
    # JAL landede her
    li a0, 'J'
    li a7, 2
    ecall

    # JALR: hop tilbage til ra+4
    jr ra

exitprog:
    li a7, 3
    ecall
