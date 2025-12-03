RISCV_CC = riscv64-unknown-elf-gcc
ARCH     = -march=rv32im -mabi=ilp32
CFLAGS   = -O0

all: test1.elf

test1.elf: test1.c
	$(RISCV_CC) $(ARCH) $(CFLAGS) test1.c -o test1.elf

clean:
	rm -f *.elf *.o
