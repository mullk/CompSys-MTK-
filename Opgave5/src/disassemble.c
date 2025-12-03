#include "disassemble.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#define OPCODE_MASK 0x7F
#define FUNC3_MASK 0x7000
#define RD_MASK 0xF80
#define RS1_MASK 0xF8000
#define RS2_MASK 0x1F00000

#define I_TYPE_IMM_MASK 0xFFF00000
#define S_TYPE_IMM_MASK 0xFE000000
#define U_TYPE_IMM_MASK 0xFFFFF000
#define J_TYPE_IMM_MASK_1_10 0x7FE00000
#define J_TYPE_IMM_MASK_11 0x100000
#define J_TYPE_IMM_MASK_12_19 0xFF000
#define J_TYPE_IMM_MASK_20 0x80000000

#define OPCODE_LUI 0x37
#define OPCODE_AUIPC 0x17
#define OPCODE_JAL 0x6F
#define OPCODE_JALR 0x67
#define OPCODE_BRANCH 0x63
#define OPCODE_LOAD 0x3
#define OPCODE_STORE 0x23
#define OPCODE_NUMERIC_INTERMEDIARY 0x13
#define OPCODE_NUMERIC 0x33
#define OPCODE_ECALL 0x73

#define FUNC3_JALR 0x0

#define FUNC3_BEQ  0x0
#define FUNC3_BNE  0x1
#define FUNC3_BLT  0x4
#define FUNC3_BGE  0x5
#define FUNC3_BLTU 0x6
#define FUNC3_BGEU 0x7

#define FUNC3_LB 0x0
#define FUNC3_LH 0x1
#define FUNC3_LW 0x2
#define FUNC3_LBU 0x4
#define FUNC3_LHU 0x5

#define FUNC3_SB 0x0
#define FUNC3_SH 0x1
#define FUNC3_SW 0x2

#define FUNC3_ADDI  0x0
#define FUNC3_SLTI  0x2
#define FUNC3_SLTIU 0x3
#define FUNC3_XORI  0x4
#define FUNC3_ORI   0x6
#define FUNC3_ANDI  0x7
#define FUNC3_SLLI  0x1
#define FUNC3_SRLI_SRAI 0x5

#define FUNC3_ADD_SUB_MUL 0x0
#define FUNC3_SLL_MULH 0x1
#define FUNC3_SLT_MULHSU 0x2
#define FUNC3_SLTU_MULHU 0x3
#define FUNC3_XOR_DIV 0x4
#define FUNC3_SRL_SRA_DIVU 0x5
#define FUNC3_OR_REM 0x6
#define FUNC3_AND_REMU 0x7

#define REGISTER_COMMA_TRUE 1
#define REGISTER_COMMA_FALSE 0
#define REGISTER_RD  0
#define REGISTER_RS1 1
#define REGISTER_RS2 2 //ebreak, fence, fence.i, CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI og CSRRCI.

void add_mnemonic(char* mnemonic, char* result, uint8_t* used, size_t buf_size);
void decode_registre(uint32_t instruction, uint8_t registr, uint8_t comma, char* result, uint8_t* used, size_t buf_size);
void handle_i_type(uint32_t instruction, char* result, uint8_t* used, size_t buf_size);
void handle_i_type_shift(uint32_t instruction, char* result, uint8_t* used, size_t buf_size);
void handle_i_type_imm(uint32_t instruction, char* result, uint8_t* used, size_t buf_size);
void handle_i_type_load(uint32_t instruction, char* result, uint8_t* used, size_t buf_size);
void handle_s_type(uint32_t instruction, char* result, uint8_t* used, size_t buf_size);
void handle_b_type(uint32_t addr, uint32_t instruction, char* result, uint8_t* used, size_t buf_size);
void handle_u_type(uint32_t instruction, char* result, uint8_t* used, size_t buf_size);
void handle_j_type(uint32_t instruction, char* result, uint8_t* used, size_t buf_size);

void disassemble(uint32_t addr, uint32_t instruction, char* result, size_t buf_size, struct symbols* symbols){
    uint8_t used = 0;
    memset(result, '\0', buf_size);

    uint32_t opcode = (instruction & OPCODE_MASK);
    if(opcode == OPCODE_LUI){
        add_mnemonic("LUI", result, &used, buf_size);
        handle_u_type(instruction, result, &used, buf_size);
    }else if(opcode == OPCODE_AUIPC){
        add_mnemonic("AUIPC", result, &used, buf_size);
        handle_u_type(instruction, result, &used, buf_size);
    }else if(opcode == OPCODE_JAL){
        add_mnemonic("JAL", result, &used, buf_size);
        handle_j_type(instruction, result, &used, buf_size);
    }else if(opcode == OPCODE_JALR){
        uint32_t func3 = (instruction & FUNC3_MASK) >> 12;
        switch (func3){
        case FUNC3_JALR:
            add_mnemonic("JALR", result, &used, buf_size);
            handle_i_type(instruction, result, &used, buf_size);
            break;
        default:
            assert(0);
            break;
        }
    }else if(opcode == OPCODE_BRANCH){
        uint32_t func3 = (instruction & FUNC3_MASK) >> 12;
        switch (func3){
            case FUNC3_BEQ:
                add_mnemonic("BEQ", result, &used, buf_size);
                handle_b_type(addr, instruction, result, &used, buf_size);
                break;
            case FUNC3_BNE:
                add_mnemonic("BNE", result, &used, buf_size);
                handle_b_type(addr, instruction, result, &used, buf_size);
                break;
            case FUNC3_BLT:
                add_mnemonic("BLT", result, &used, buf_size);
                handle_b_type(addr, instruction, result, &used, buf_size);
                break;
            case FUNC3_BGE:
                add_mnemonic("BGE", result, &used, buf_size);
                handle_b_type(addr, instruction, result, &used, buf_size);
                break;
            case FUNC3_BLTU:
                add_mnemonic("BLTU", result, &used, buf_size);
                handle_b_type(addr, instruction, result, &used, buf_size);
                break;
            case FUNC3_BGEU:
                add_mnemonic("BGEU", result, &used, buf_size);
                handle_b_type(addr, instruction, result, &used, buf_size);
                break;
            default:
                assert(0);
                break;
        }
    }else if(opcode == OPCODE_LOAD){
        uint32_t func3 = (instruction & FUNC3_MASK) >> 12;
        switch (func3){
            case FUNC3_LB:
                add_mnemonic("LB", result, &used, buf_size);
                handle_i_type_load(instruction, result, &used, buf_size);
                break;
            case FUNC3_LH:
                add_mnemonic("LH", result, &used, buf_size);
                handle_i_type_load(instruction, result, &used, buf_size);
                break;
            case FUNC3_LW:
                add_mnemonic("LW", result, &used, buf_size);
                handle_i_type_load(instruction, result, &used, buf_size);
                break;
            case FUNC3_LBU:
                add_mnemonic("LBU", result, &used, buf_size);
                handle_i_type_load(instruction, result, &used, buf_size);
                break;
            case FUNC3_LHU:
                add_mnemonic("LHU", result, &used, buf_size);
                handle_i_type_load(instruction, result, &used, buf_size);
                break;
            default:
                break;
        }
    }else if(opcode == OPCODE_STORE){
        uint32_t func3 = (instruction & FUNC3_MASK) >> 12;
        switch (func3){
            case FUNC3_SB:
                add_mnemonic("SB", result, &used, buf_size);
                handle_s_type(instruction, result, &used, buf_size);
                break;
            case FUNC3_SH:
                add_mnemonic("SH", result, &used, buf_size);
                handle_s_type(instruction, result, &used, buf_size);
                break;
            case FUNC3_SW:
                add_mnemonic("SW", result, &used, buf_size);
                handle_s_type(instruction, result, &used, buf_size);
                break;
            default:
                break;
        }
    }else if(opcode == OPCODE_NUMERIC_INTERMEDIARY){
        uint32_t func3 = (instruction & FUNC3_MASK) >> 12;
        switch (func3){
            case FUNC3_ADDI:
                add_mnemonic("ADDI", result, &used, buf_size);
                handle_i_type(instruction, result, &used, buf_size);
                break;
            case FUNC3_SLTI:
                add_mnemonic("SLTI", result, &used, buf_size);
                handle_i_type(instruction, result, &used, buf_size);
                break;
            case FUNC3_SLTIU:
                add_mnemonic("SLTIU", result, &used, buf_size);
                handle_i_type(instruction, result, &used, buf_size);
                break;
            case FUNC3_XORI:
                add_mnemonic("XORI", result, &used, buf_size);
                handle_i_type(instruction, result, &used, buf_size);
                break;
            case FUNC3_ORI:
                add_mnemonic("ORI", result, &used, buf_size);
                handle_i_type(instruction, result, &used, buf_size);
                break;
            case FUNC3_ANDI:
                add_mnemonic("ANDI", result, &used, buf_size);
                handle_i_type(instruction, result, &used, buf_size);
                break;
            case FUNC3_SLLI:
                add_mnemonic("SSLI", result, &used, buf_size);
                handle_i_type_shift(instruction, result, &used, buf_size);
                break;
            case FUNC3_SRLI_SRAI:{
                uint32_t logical_or_arithmetic = instruction & 0x40000000;
                if(logical_or_arithmetic == 0x40000000){
                    add_mnemonic("SRAI", result, &used, buf_size);
                }else{
                    add_mnemonic("SRLI", result, &used, buf_size);
                }
                handle_i_type_shift(instruction, result, &used, buf_size);
                break;
            }
            default:
                assert(0);
                break;
        }
    }else if(opcode == OPCODE_NUMERIC){
        uint32_t func3 = (instruction & FUNC3_MASK) >> 12;
        switch (func3){
            case FUNC3_ADD_SUB_MUL:{
                uint32_t add_mul_or_sub = instruction & 0x40000000;
                if(add_mul_or_sub == 0x40000000){
                    add_mnemonic("SUB", result, &used, buf_size);
                }else{
                    uint32_t add_or_mul = instruction & 0x2000000;
                    if(add_or_mul == 0x2000000){
                        add_mnemonic("MUL", result, &used, buf_size);
                    }else{
                        add_mnemonic("ADD", result, &used, buf_size);
                    }
                }
                handle_i_type_imm(instruction, result, &used, buf_size);
                break;
            }
            case FUNC3_SLL_MULH:{
                uint32_t sll_or_mulh = instruction & 0x2000000;
                if(sll_or_mulh == 0x2000000){
                    add_mnemonic("MULH", result, &used, buf_size);
                }else{
                    add_mnemonic("SLL", result, &used, buf_size);
                }
                handle_i_type_imm(instruction, result, &used, buf_size);
                break;
            }
            case FUNC3_SLT_MULHSU:{
                uint32_t slt_or_mulhsu =  instruction & 0x2000000;
                if(slt_or_mulhsu == 0x2000000){
                    add_mnemonic("MULHSU", result, &used, buf_size);
                }else{
                    add_mnemonic("SLT", result, &used, buf_size);
                }
                handle_i_type_imm(instruction, result, &used, buf_size);
                break;
            }
            case FUNC3_SLTU_MULHU:{
                uint32_t sltu_or_mulhu =  instruction & 0x2000000;
                if(sltu_or_mulhu == 0x2000000){
                    add_mnemonic("MULHU", result, &used, buf_size);
                }else{
                    add_mnemonic("SLTU", result, &used, buf_size);
                }
                handle_i_type_imm(instruction, result, &used, buf_size);
                break;
            }
            case FUNC3_XOR_DIV:{
                uint32_t xor_or_div = instruction & 0x2000000;
                if(xor_or_div == 0x2000000){
                    add_mnemonic("DIV", result, &used, buf_size);
                }else{
                    add_mnemonic("XOR", result, &used, buf_size);
                }
                handle_i_type_imm(instruction, result, &used, buf_size);
                break;
            }
            case FUNC3_SRL_SRA_DIVU:{
                uint32_t logical_divu_or_arithmetic = instruction & 0x40000000;
                if(logical_divu_or_arithmetic == 0x40000000){
                    add_mnemonic("SRA", result, &used, buf_size);
                }else{
                    uint32_t logical_or_divu = instruction & 0x2000000;
                    if(logical_or_divu == 0x2000000){
                    add_mnemonic("DIVU", result, &used, buf_size);
                    }else{
                        add_mnemonic("SRL", result, &used, buf_size);
                    }
                }
                handle_i_type_imm(instruction, result, &used, buf_size);
                break;
            }
            case FUNC3_OR_REM:{
                uint32_t or_or_rem = instruction & 0x2000000;
                if(or_or_rem == 0x2000000){
                    add_mnemonic("REM", result, &used, buf_size);
                }else{
                    add_mnemonic("OR", result, &used, buf_size);
                }
                handle_i_type_imm(instruction, result, &used, buf_size);
                break;
            }
            case FUNC3_AND_REMU:{
                uint32_t and_or_remu = instruction & 0x2000000;
                if(and_or_remu == 0x2000000){
                    add_mnemonic("REMU", result, &used, buf_size);
                }else{
                    add_mnemonic("AND", result, &used, buf_size);
                }
                handle_i_type_imm(instruction, result, &used, buf_size);
                break;
            }
            default:
                assert(0);
                break;
        }
    }else if(opcode == OPCODE_ECALL){
        add_mnemonic("ECALL", result, &used, buf_size);
    }
}

void convert_registry(uint32_t reg_code, char* reg){
    memset(reg, '\0', 5);
    switch (reg_code)
    {
    case 0x00:
        memcpy(reg, "zero", 4);
        break;
    case 0x01:
        memcpy(reg, "ra", 2);
        break;
    case 0x02:
        memcpy(reg, "sp", 2);
        break;
    case 0x03:
        memcpy(reg, "gp", 2);
        break;
    case 0x04:
        memcpy(reg, "tp", 2);
        break;
    case 0x05:
        memcpy(reg, "t0", 2);
        break;
    case 0x06:
        memcpy(reg, "t1", 2);
        break;
    case 0x07:
        memcpy(reg, "t2", 2);
        break;
    case 0x08:
        memcpy(reg, "s0", 2);
        break;
    case 0x09:
        memcpy(reg, "s1", 2);
        break;
    case 0x0A:
        memcpy(reg, "a0", 2);
        break;
    case 0x0B:
        memcpy(reg, "a1", 2);
        break;
    case 0x0C:
        memcpy(reg, "a2", 2);
        break;
    case 0x0D:
        memcpy(reg, "a3", 2);
        break;
    case 0x0E:
        memcpy(reg, "a4", 2);
        break;
    case 0x0F:
        memcpy(reg, "a5", 2);
        break;
    case 0x10:
        memcpy(reg, "a6", 2);
        break;
    case 0x11:
        memcpy(reg, "a7", 2);
        break;
    case 0x12:
        memcpy(reg, "s2", 2);
        break;
    case 0x13:
        memcpy(reg, "s3", 2);
        break;
    case 0x14:
        memcpy(reg, "s4", 2);
        break;
    case 0x15:
        memcpy(reg, "s5", 2);
        break;
    case 0x16:
        memcpy(reg, "s6", 2);
        break;
    case 0x17:
        memcpy(reg, "s7", 2);
        break;
    case 0x18:
        memcpy(reg, "s8", 2);
        break;
    case 0x19:
        memcpy(reg, "s9", 2);
        break;
    case 0x1A:
        memcpy(reg, "s10", 3);
        break;
    case 0x1B:
        memcpy(reg, "s11", 3);
        break;
    case 0x1C:
        memcpy(reg, "t3", 2);
        break;
    case 0x1D:
        memcpy(reg, "t4", 2);
        break;
    case 0x1E:
        memcpy(reg, "t5", 2);
        break;
    case 0x1F:
        memcpy(reg, "t6", 2);
        break;
    default:
        assert(0);
        break;
    }
}

void add_mnemonic(char* mnemonic, char* result, uint8_t* used, size_t buf_size){
    uint8_t FIXED_LEN = 8;
    memcpy(result + *used, mnemonic, strlen(mnemonic));
    *used += strlen(mnemonic);
    for(uint8_t i = 0; i < FIXED_LEN - strlen(mnemonic); i++){
        memcpy(result + *used, " ", 1);
        (*used)++;
    }
}

void decode_registre(uint32_t instruction, uint8_t registr, uint8_t comma, char* result, uint8_t* used, size_t buf_size){
    uint32_t reg_int;
    switch (registr)
    {
    case REGISTER_RD:
        reg_int = (instruction & RD_MASK) >> 7;
        break;
    case REGISTER_RS1:
        reg_int = (instruction & RS1_MASK) >> 15;
        break;
    case REGISTER_RS2:
        reg_int = (instruction & RS2_MASK) >> 20;
        break;
    default:
        assert(0);
        break;
    }
    char reg_char[5];
    convert_registry(reg_int, reg_char);
    memcpy(&result[*used], reg_char, strlen(reg_char));
    *used += strlen(reg_char);

    if(comma == REGISTER_COMMA_TRUE){
        result[*used] = ',';
        (*used)++;
    }
}

void i_type_immediat_decode(uint32_t instruction, uint32_t* imm){
    uint32_t imm_mask = (instruction & I_TYPE_IMM_MASK);
    *imm = imm_mask >> 20;
    if((*imm & 0x800) == 0x800){
        *imm = *imm & 0x7FF;
        *imm = *imm | 0xFFFFF800;
    }
}

void convert_imm_to_str(uint32_t imm, char* form, char* result, uint8_t* used, size_t buf_size){
    char imm_char[11];
    memset(imm_char, '\0', 11);
    sprintf(imm_char, form, imm);

    memcpy(&result[*used], imm_char, strlen(imm_char));
    (*used) += strlen(imm_char);
}

void handle_i_type(uint32_t instruction, char* result, uint8_t* used, size_t buf_size){
    decode_registre(instruction, REGISTER_RD, 1, result, used, buf_size);
    decode_registre(instruction, REGISTER_RS1, 1, result, used, buf_size);

    uint32_t imm = 0;
    i_type_immediat_decode(instruction, &imm);
    convert_imm_to_str(imm, "%i", result, used, buf_size);
}
void handle_i_type_shift(uint32_t instruction, char* result, uint8_t* used, size_t buf_size){
    decode_registre(instruction, REGISTER_RD, REGISTER_COMMA_TRUE, result, used, buf_size);
    decode_registre(instruction, REGISTER_RS1, REGISTER_COMMA_TRUE, result, used, buf_size);

    uint32_t shift = (instruction & 0xF00000) >> 20;
    sprintf(&result[*used], "%#x", shift);
}

void handle_i_type_imm(uint32_t instruction, char* result, uint8_t* used, size_t buf_size){
    decode_registre(instruction, REGISTER_RD, REGISTER_COMMA_TRUE, result, used, buf_size);
    decode_registre(instruction, REGISTER_RS1, REGISTER_COMMA_TRUE, result, used, buf_size);
    decode_registre(instruction, REGISTER_RS2, REGISTER_COMMA_FALSE, result, used, buf_size);
}

void handle_i_type_load(uint32_t instruction, char* result, uint8_t* used, size_t buf_size){
    decode_registre(instruction, REGISTER_RD, REGISTER_COMMA_TRUE, result, used, buf_size);
    
    result[*used] = '(';
    (*used)++;
    uint32_t imm = 0;
    i_type_immediat_decode(instruction, &imm);
    convert_imm_to_str(imm, "%i", result, used, buf_size);
    result[*used] = ')';
    (*used)++;

    decode_registre(instruction, REGISTER_RS1, REGISTER_COMMA_FALSE, result, used, buf_size);
}

void handle_s_type(uint32_t instruction, char* result, uint8_t* used, size_t buf_size){
    decode_registre(instruction, REGISTER_RS2, REGISTER_COMMA_TRUE, result, used, buf_size);

    result[*used] = '(';
    (*used)++;
    uint32_t imm =(instruction & S_TYPE_IMM_MASK) >> 21;
    uint32_t imm_rd = (instruction & RD_MASK) >> 7;
    imm = imm | imm_rd;
    if((imm & 0x800) == 0x800){
        imm = imm & 0x7FF;
        imm = imm | 0xFFFFF800;
    }
    convert_imm_to_str(imm, "%i", result, used, buf_size);
    result[*used] = ')';
    (*used)++;

    decode_registre(instruction, REGISTER_RS1, REGISTER_COMMA_FALSE, result, used, buf_size);
}

void handle_b_type(uint32_t addr, uint32_t instruction, char* result, uint8_t* used, size_t buf_size){
    decode_registre(instruction, REGISTER_RS1, REGISTER_COMMA_TRUE, result, used, buf_size);
    decode_registre(instruction, REGISTER_RS2, REGISTER_COMMA_TRUE, result, used, buf_size);
    
    uint32_t imm =(instruction & S_TYPE_IMM_MASK) >> 21;
    uint32_t imm_rd = (instruction & RD_MASK) >> 7;
    uint32_t imm_rd_low = imm_rd & 0x1;
    imm_rd_low = imm_rd_low << 11;
    imm_rd &= 0xFFFFFFFE;
    imm_rd |= imm_rd_low;

    imm = imm | imm_rd;
    if((instruction & 0x80000000) == 0x80000000){
        imm = imm & 0xFFF;
        imm = imm | 0xFFFFF000;
    }

    convert_imm_to_str(imm, "%#x", result, used, buf_size);
}
void handle_u_type(uint32_t instruction, char* result, uint8_t* used, size_t buf_size){
    decode_registre(instruction, REGISTER_RD, REGISTER_COMMA_TRUE, result, used, buf_size);

    uint32_t imm = (instruction & U_TYPE_IMM_MASK) >> 12;
    sprintf(&result[*used], "%#x", imm);
}

void handle_j_type(uint32_t instruction, char* result, uint8_t* used, size_t buf_size){
    decode_registre(instruction, REGISTER_RD, REGISTER_COMMA_TRUE, result, used, buf_size);
    //TODO fix immediate
    uint32_t imm = (instruction & J_TYPE_IMM_MASK_1_10) >> 21;
    imm = imm | (instruction & J_TYPE_IMM_MASK_11) >> 10;
    imm = imm | (instruction & J_TYPE_IMM_MASK_12_19) >> 1;
    imm = imm | (instruction & J_TYPE_IMM_MASK_20);

    sprintf(&result[*used], "%#x", imm);
}