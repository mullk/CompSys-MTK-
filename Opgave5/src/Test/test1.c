void putchar_rv(char c) {
    asm volatile(
        "mv a0, %0\n"
        "li a7, 2\n"
        "ecall\n"
        :
        :"r"(c)
    );
}

void exit_rv() {
    asm volatile(
        "li a7, 3\n"
        "ecall\n"
    );
}

int main() {
    putchar_rv('A');
    putchar_rv('\n');
    exit_rv();
    return 0;
}
