#include "simulate.h"
#include "memory.h"
#include "read_elf.h"
#include <stdint.h>
#include <stdio.h>

// --- Hjælpefunktioner ------------------------------------------------------

int32_t sign_extend(int32_t value, int bits) {
    int32_t mask = 1 << (bits - 1);
    return (value ^ mask) - mask;
}

uint32_t get_opcode(uint32_t inst) { return inst & 0x7F; }
uint32_t get_rd(uint32_t inst)     { return (inst >> 7) & 0x1F; }
uint32_t get_f3(uint32_t inst)     { return (inst >> 12) & 0x7; }
uint32_t get_rs1(uint32_t inst)    { return (inst >> 15) & 0x1F; }
uint32_t get_rs2(uint32_t inst)    { return (inst >> 20) & 0x1F; }
uint32_t get_f7(uint32_t inst)     { return (inst >> 25) & 0x7F; }

// --- Immediate rekonstruktion (I/S/B/U/J) ---------------------------------

int32_t imm_I(uint32_t inst) {
    return sign_extend(inst >> 20, 12);
}

int32_t imm_S(uint32_t inst) {
    int32_t imm = ((inst >> 25) << 5) | ((inst >> 7) & 0x1F);
    return sign_extend(imm, 12);
}

int32_t imm_B(uint32_t inst) {
    int32_t imm =
        ((inst >> 31) & 1) << 12 |
        ((inst >> 7) & 1) << 11 |
        ((inst >> 25) & 0x3F) << 5 |
        ((inst >> 8) & 0xF) << 1;

    return sign_extend(imm, 13);
}

int32_t imm_U(uint32_t inst) {
    return (int32_t)(inst & 0xFFFFF000);
}

int32_t imm_J(uint32_t inst) {
    int32_t imm =
        ((inst >> 31) & 1) << 20 |
        ((inst >> 21) & 0x3FF) << 1 |
        ((inst >> 20) & 1) << 11 |
        ((inst >> 12) & 0xFF) << 12;

    return sign_extend(imm, 21);
}

// --- Systemkald -------------------------------------------------------------

int handle_ecall(int32_t regs[]) {
    int a7 = regs[17]; // x17 = A7
    int a0 = regs[10]; // x10 = A0

    if (a7 == 1) { // getchar
        int c = getchar();
        regs[10] = c;
        return 0;
    }
    if (a7 == 2) { // putchar
        putchar(a0);
        return 0;
    }
    if (a7 == 3 || a7 == 93) { // exit
        return 1;
    }
    return 0;
}

// --- Hovedsimulator ---------------------------------------------------------
//Det skal ikke læses som en deklaration af en struct. De to første ord = return type declaration. 
struct Stat simulate(struct memory *mem, int start_addr, FILE *log_file, struct symbols* symbols)
{
    struct Stat st = {0};
    int32_t regs[32] = {0};

    uint32_t pc = (uint32_t)start_addr;

    while (1) {
        uint32_t inst = (uint32_t)memory_rd_w(mem, pc);
        uint32_t next_pc = pc + 4;
        st.insns++;

        uint32_t opcode = get_opcode(inst);
        uint32_t f3 = get_f3(inst);
        uint32_t f7 = get_f7(inst);
        uint32_t rd = get_rd(inst);
        uint32_t rs1 = get_rs1(inst);
        uint32_t rs2 = get_rs2(inst);

        int32_t v1 = regs[rs1];
        int32_t v2 = regs[rs2];

        if (log_file) {
            fprintf(log_file, "%08x : %08x\n", pc, inst);
        }

        // --- LUI / AUIPC ----------------------------------------------------
        if (opcode == 0x37) {                 // LUI
            regs[rd] = imm_U(inst);
        }
        else if (opcode == 0x17) {            // AUIPC
            regs[rd] = pc + imm_U(inst);
        }

        // --- JAL -------------------------------------------------------------
        else if (opcode == 0x6F) {
            regs[rd] = pc + 4;
            next_pc = pc + imm_J(inst);
        }

        // --- JALR ------------------------------------------------------------
        else if (opcode == 0x67) {
            regs[rd] = pc + 4;
            next_pc = (v1 + imm_I(inst)) & ~1;
        }

        // --- BRANCH ----------------------------------------------------------
        else if (opcode == 0x63) {
            int32_t imm = imm_B(inst);
            if (f3 == 0 && v1 == v2) next_pc = pc + imm;           // beq
            else if (f3 == 1 && v1 != v2) next_pc = pc + imm;      // bne
            else if (f3 == 4 && v1 < v2)  next_pc = pc + imm;      // blt
            else if (f3 == 5 && v1 >= v2) next_pc = pc + imm;      // bge
            else if (f3 == 6 && (uint32_t)v1 < (uint32_t)v2)  next_pc = pc + imm; // bltu
            else if (f3 == 7 && (uint32_t)v1 >= (uint32_t)v2) next_pc = pc + imm; // bgeu
        }

        // --- LOAD ------------------------------------------------------------
        else if (opcode == 0x03) {
            int32_t addr = v1 + imm_I(inst);

            if      (f3 == 0)  regs[rd] = (int8_t) memory_rd_b(mem, addr);
            else if (f3 == 4)  regs[rd] = (uint8_t) memory_rd_b(mem, addr);
            else if (f3 == 1)  regs[rd] = (int16_t) memory_rd_h(mem, addr);
            else if (f3 == 5)  regs[rd] = (uint16_t) memory_rd_h(mem, addr);
            else if (f3 == 2)  regs[rd] = memory_rd_w(mem, addr);
        }

        // --- STORE -----------------------------------------------------------
        else if (opcode == 0x23) {
            int32_t addr = v1 + imm_S(inst);

            if      (f3 == 0) memory_wr_b(mem, addr, v2);
            else if (f3 == 1) memory_wr_h(mem, addr, v2);
            else if (f3 == 2) memory_wr_w(mem, addr, v2);
        }

        // --- I-TYPE ALU ------------------------------------------------------
        else if (opcode == 0x13) {
            int32_t imm = imm_I(inst);

            if      (f3 == 0) regs[rd] = v1 + imm;     // addi
            else if (f3 == 2) regs[rd] = v1 < imm;      // slti
            else if (f3 == 3) regs[rd] = (uint32_t)v1 < (uint32_t)imm; // sltiu
            else if (f3 == 4) regs[rd] = v1 ^ imm;      // xori
            else if (f3 == 6) regs[rd] = v1 | imm;      // ori
            else if (f3 == 7) regs[rd] = v1 & imm;      // andi

            else if (f3 == 1) { // SLLI
                int shamt = (inst >> 20) & 0x1F;
                regs[rd] = v1 << shamt;
            }
            else if (f3 == 5) {
                int shamt = (inst >> 20) & 0x1F;
                if ((inst >> 30) & 1) regs[rd] = v1 >> shamt;          // SRAI
                else                  regs[rd] = (uint32_t)v1 >> shamt; // SRLI
            }
        }

        // --- R-TYPE (ADD/SUB/MUL/DIV...) ------------------------------------
        else if (opcode == 0x33) {
            if (f3 == 0) {
                if (f7 == 0x20) regs[rd] = v1 - v2;     // sub
                else if (f7 == 0x01) regs[rd] = v1 * v2; // mul
                else                regs[rd] = v1 + v2; // add
            }
            else if (f3 == 1) {
                if (f7 == 0x01) regs[rd] = ((int64_t)v1 * (int64_t)v2) >> 32; // mulh
                else            regs[rd] = v1 << (v2 & 0x1F);                // sll
            }
            else if (f3 == 2) {
                if (f7 == 0x01) regs[rd] = ((int64_t)v1 * (uint64_t)v2) >> 32; // mulhsu
                else            regs[rd] = v1 < v2;                            // slt
            }
            else if (f3 == 3) {
                if (f7 == 0x01) regs[rd] = ((uint64_t)v1 * (uint64_t)v2) >> 32; // mulhu
                else            regs[rd] = (uint32_t)v1 < (uint32_t)v2;         // sltu
            }
            else if (f3 == 4) {
                if (f7 == 0x01) regs[rd] = v1 / v2; // div
                else            regs[rd] = v1 ^ v2; // xor
            }
            else if (f3 == 5) {
                if (f7 == 0x20) regs[rd] = v1 >> (v2 & 0x1F);                  // sra
                else if (f7 == 0x01) regs[rd] = (uint32_t)v1 / (uint32_t)v2;   // divu
                else                 regs[rd] = (uint32_t)v1 >> (v2 & 0x1F);    // srl
            }
            else if (f3 == 6) {
                if (f7 == 0x01) regs[rd] = v1 % v2; // rem
                else            regs[rd] = v1 | v2; // or
            }
            else if (f3 == 7) {
                if (f7 == 0x01) regs[rd] = (uint32_t)v1 % (uint32_t)v2; // remu
                else            regs[rd] = v1 & v2;                    // and
            }
        }

        // --- ECALL -----------------------------------------------------------
        else if (opcode == 0x73) {
            if (handle_ecall(regs))
                return st;
        }

        // --- Opdater PC ------------------------------------------------------
        pc = next_pc;

        // x0 skal altid være 0
        regs[0] = 0;
    }

    return st;
}
