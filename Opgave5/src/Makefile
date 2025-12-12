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
	rm -rf *.o sim  vgcore* dis_test


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

tests_sim: $(TEST_ELF)
	@echo "Alle test .elf filer er bygget!"


clean_tests:
	rm -f Test/*.o Test/*.elf

PREDICTOR_DIR=../predictor-benchmarks
TEST_DIS_DIR=dis_test
CROSS_DIR=../network-cross-compiler
TEST_FILES=$(shell cd $(PREDICTOR_DIR) && find . -name '*.elf' | awk -F './' '{print $$2}')
.NOTINTERMEDIATE:

%.elf:
	@cp $(PREDICTOR_DIR)/$@ $@

$(TEST_DIS_DIR)/objdump/%.dis: ./%.elf
	@mkdir -p $(@D)
	@$(CROSS_DIR)/objdump.py -S $< | tail -n+7 | awk -F ' ' '{if ($$3 == ""){ print $$0 } else { printf "%-8s %s\n", $$3, $$4}}' > $(@D)/$(@F:.elf=.dis)

$(TEST_DIS_DIR)/sim/%.dis: ./%.elf
	@mkdir -p $(@D)
	@./sim $< -d | tail -n+3 | awk -F ' ' '{if ($$3 == ""){ print $$0 } else if ($$4 == "Unknown") {} else { printf "%-8s %s\n", $$4, $$5}}' > $(@D)/$(@F:.elf=.dis)

%.t: $(addprefix $(TEST_DIS_DIR)/objdump/,%.dis) $(addprefix $(TEST_DIS_DIR)/sim/,%.dis)
	@./test-disassembler.sh $(TEST_DIS_DIR)/objdump/$(@:.t=.dis) $(TEST_DIS_DIR)/sim/$(@:.t=.dis) $(@:.t=.elf)

test_dis: $(TEST_FILES:.elf=.t)
	@rm $(TEST_FILES)