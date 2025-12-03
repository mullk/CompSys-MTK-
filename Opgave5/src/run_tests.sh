#!/bin/bash

AS="riscv64-unknown-elf-as -march=rv32im -mabi=ilp32"
LD="riscv64-unknown-elf-ld -m elf32lriscv"

TEST_DIR="Test"

echo "Bruger RV32 toolchain:"
echo "  AS = $AS"
echo "  LD = $LD"
echo

for sfile in $TEST_DIR/*.s; do
    base=${sfile%.s}
    echo "--------------------------------"
    echo "Bygger og kører: $sfile"
    
    $AS "$sfile" -o "$base.o"
    $LD "$base.o" -o "$base.elf"

    echo "Kører $base.elf ..."
    ./sim "$base.elf"
    echo
done

echo "Alle tests færdige!"
