# GCC=gcc -g -Wall -Wextra -pedantic -std=gnu11 
GCC=gcc -g -Wall -Wextra -pedantic -std=gnu11 -O

all: sim
rebuild: clean all

# sim nedds simulate and disassemble to work!
sim: *.c *.h
	$(GCC) *.c -o sim 

zip: ../src.zip

../src.zip: clean
	cd .. && zip -r src.zip src/Makefile src/*.c src/*.h

clean:
	rm -rf *.o sim  vgcore*


## Brug RISC-V assembleren riscv64-unknown-elf-as til at oversætte .s assembly-filer til .o objektfiler.
AS = riscv64-unknown-elf-as

## Brug RISC-V linkeren til at samle objektfiler og danne en .elf binary.
LD = riscv64-unknown-elf-ld

TEST_DIR = Test
TEST_S   = $(wildcard $(TEST_DIR)/*.s)
TEST_O   = $(TEST_S:.s=.o)
TEST_ELF = $(TEST_S:.s=.elf)

$(TEST_DIR)/%.o: $(TEST_DIR)/%.s
	$(AS) -march=rv32im -mabi=ilp32 $< -o $@

$(TEST_DIR)/%.elf: $(TEST_DIR)/%.o
	$(LD) -m elf32lriscv $< -o $@

tests: $(TEST_ELF)
	@echo "Alle test .elf filer er bygget!"


clean_tests:
	rm -f Test/*.o Test/*.elf




