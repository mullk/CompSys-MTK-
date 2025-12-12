#include "simulate.h"
#include "memory.h"
#include "read_elf.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

struct Stat simulate(struct memory *mem, int start_addr, FILE *log_file, struct symbols* symbols)
{
    struct Stat st = {0};
    int32_t regs[32] = {0};
    uint32_t pc = (uint32_t)start_addr;

    // --- Branch prediction status ------------------------------------------
    long nt_errors = 0;
    long btfnt_errors = 0;
    #define BIMODAL_SIZES 4
    const int bimodal_sizes[BIMODAL_SIZES] = {256, 1024, 4096, 16384};
    long bimodal_errors[BIMODAL_SIZES] = {0};
    long gshare_errors[BIMODAL_SIZES] = {0};
    uint8_t *bimodal[BIMODAL_SIZES];
    uint8_t *gshare[BIMODAL_SIZES];
    for(int i = 0; i < BIMODAL_SIZES; i++) {
        bimodal[i] = malloc(bimodal_sizes[i] * sizeof(uint8_t));
        gshare[i] = malloc(bimodal_sizes[i] * sizeof(uint8_t));
        memset(bimodal[i], 2, bimodal_sizes[i]);
        memset(gshare[i], 2, bimodal_sizes[i]);
    }

    uint32_t ghr = 0;

    long branch_count = 0; // tæller branch-instruktioner

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

        int is_branch = 0;
        int taken = 0;
        int32_t branch_target = 0;

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
            is_branch = 1;
            taken = 1;
            branch_target = next_pc;
            branch_count++;
        }

        // --- JALR ------------------------------------------------------------
        else if (opcode == 0x67) {
            regs[rd] = pc + 4;
            next_pc = (v1 + imm_I(inst)) & ~1;
            is_branch = 1;
            taken = 1;
            // Hvis taken =1, så er der foretaget et hop, ellers hvis ikke = 0 
            branch_target = next_pc;
            branch_count++;
        }

        // --- BRANCH ----------------------------------------------------------
        else if (opcode == 0x63) {
            int32_t imm = imm_B(inst);
            branch_target = pc + imm;
            branch_count++;
            if (f3 == 0 && v1 == v2) next_pc = branch_target, taken = 1;           // beq
            else if (f3 == 1 && v1 != v2) next_pc = branch_target, taken = 1;      // bne
            else if (f3 == 4 && v1 < v2)  next_pc = branch_target, taken = 1;      // blt
            else if (f3 == 5 && v1 >= v2) next_pc = branch_target, taken = 1;      // bge
            else if (f3 == 6 && (uint32_t)v1 < (uint32_t)v2)  next_pc = branch_target, taken = 1; // bltu
            else if (f3 == 7 && (uint32_t)v1 >= (uint32_t)v2)  next_pc = branch_target, taken = 1; // bgeu
            is_branch = 1;
        }

        // --- Branch prediction ---------------------------------------------
        if(is_branch){
            // NT predictor
            if(taken) nt_errors++;
            // BTFNT predictor
            int backward = (branch_target - (int32_t)pc) < 0;
            if(taken != (backward ? 1 : 0)) btfnt_errors++;

            // Bimodal and gShare predictors
            for(int i=0;i<BIMODAL_SIZES;i++){
                int idx = (pc >> 2) & (bimodal_sizes[i]-1);
                int predicted = bimodal[i][idx]>=2?1:0;
                if(predicted != taken) bimodal_errors[i]++;
                if(taken){ if(bimodal[i][idx]<3) bimodal[i][idx]++; } 
                else { if(bimodal[i][idx]>0) bimodal[i][idx]--; }

                idx = ((pc >> 2) ^ ghr) & (bimodal_sizes[i]-1);
                predicted = gshare[i][idx]>=2?1:0;
                if(predicted != taken) gshare_errors[i]++;
                if(taken){ if(gshare[i][idx]<3) gshare[i][idx]++; } 
                else { if(gshare[i][idx]>0) gshare[i][idx]--; }
            }
            ghr = ((ghr << 1) | (taken?1:0)) & 0xFFFF;
        }

        // --- Logging --------------------------------------------------------
        if(log_file){
            fprintf(log_file, "%5ld => %08x : %08x", st.insns, pc, inst);
            if(is_branch) fprintf(log_file, "{%s}", taken ? "T" : "NT");
            fprintf(log_file, "\n");
        }

        // --- LOAD ------------------------------------------------------------
        if(opcode == 0x03){
            int32_t addr = v1 + imm_I(inst);
            if      (f3 == 0) regs[rd] = (int8_t) memory_rd_b(mem, addr);
            else if (f3 == 4) regs[rd] = (uint8_t) memory_rd_b(mem, addr);
            else if (f3 == 1) regs[rd] = (int16_t) memory_rd_h(mem, addr);
            else if (f3 == 5) regs[rd] = (uint16_t) memory_rd_h(mem, addr);
            else if (f3 == 2) regs[rd] = memory_rd_w(mem, addr);
            if(log_file && rd != 0) fprintf(log_file, "R[%2d] <- %x\n", rd, regs[rd]);
        }

        // --- STORE -----------------------------------------------------------
        else if(opcode == 0x23){
            int32_t addr = v1 + imm_S(inst);
            if      (f3 == 0) memory_wr_b(mem, addr, v2);
            else if (f3 == 1) memory_wr_h(mem, addr, v2);
            else if (f3 == 2) memory_wr_w(mem, addr, v2);
            if(log_file) fprintf(log_file, "%08x <- %x\n", addr, v2);
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
        else if(opcode == 0x73){
            if(handle_ecall(regs)) break;
        }

        // --- Opdater PC ------------------------------------------------------
        pc = next_pc;
        regs[0] = 0;
    }

    // --- Print statements --------------------------------------------------
    printf("Simulated %ld instructions\n", st.insns);
    printf("Kørte Branches: %ld\n", branch_count);
    printf("NT errors: %ld\n", nt_errors);
    printf("BTFNT errors: %ld\n", btfnt_errors);
    for(int i=0;i<BIMODAL_SIZES;i++) 
        printf("Bimodal[%d] errors: %ld\n", 
        bimodal_sizes[i], bimodal_errors[i]);
    for(int i=0;i<BIMODAL_SIZES;i++) 
        printf("gShare[%d] errors: %ld\n", 
        bimodal_sizes[i], gshare_errors[i]);
    for(int i=0;i<BIMODAL_SIZES;i++)
        { free(bimodal[i]); free(gshare[i]); }

    return st;
}
