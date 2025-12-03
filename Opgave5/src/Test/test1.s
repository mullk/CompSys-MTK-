    .globl _start
_start:
    # putchar('A')
    li a0, 65       # ASCII 'A'
    li a7, 2        # ECALL: putchar
    ecall

    # putchar('\n')
    li a0, 10
    li a7, 2
    ecall

    # exit
    li a7, 3
    ecall
