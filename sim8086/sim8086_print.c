#include "sim8086.h"
#include "sim8086_print.h"

char* get_mnemonic(u32 idx) {
    char *mnemonics[] = {
            "",
            "mov",
            "add",
            "sub",
            "cmp",
            "je",
            "jl",
            "jle",
            "jb",
            "jbe",
            "jp",
            "jo",
            "js",
            "jne",
            "jnl",
            "jnle",
            "jnb",
            "jnbe",
            "jnp",
            "jno",
            "jns",
            "loop",
            "loopz",
            "loopnz",
            "jcxz",
    };
    return mnemonics[idx];
}

char* get_reg_name(u32 reg_idx, u32 part_idx) {
    char *reg_names[][3] = {
            {"",   "",   ""},
            {"al", "ah", "ax"},
            {"bl", "bh", "bx"},
            {"cl", "ch", "cx"},
            {"dl", "dh", "dx"},
            {"sp", "sp", "sp"},
            {"bp", "bp", "bp"},
            {"si", "si", "si"},
            {"di", "di", "di"},
    };
    return reg_names[reg_idx][part_idx];
}

char* get_effective_address_base_name(EffectiveAddress address) {
    char *rm_base[] = {
            "",
            "bx+si",
            "bx+di",
            "bp+si",
            "bp+di",
            "si",
            "di",
            "bp",
            "bx",
    };
    return rm_base[address.base];
}

void print_operand(Instruction* inst, u8 operand_num, FILE* dest) {
    Operand operand = inst->operands[operand_num];
    OperandType type = operand.kind;
    switch (type) {
        case OperandRegister: {
            RegisterAccess reg = operand.reg;
            fprintf(dest, "%s", get_reg_name(reg.index, (reg.count == 2) ? 2 : reg.offset));
            break;
        }
        case OperandMemory: {
            u32 wide = inst->flags & FlagWide;
            EffectiveAddress address = operand.address;
            if (inst->operands[0].kind != OperandRegister) {
                fprintf(dest, "%s ", wide ? "word" : "byte");
            }
            fprintf(dest, "[%s", get_effective_address_base_name(address));
            if (address.base == Ea_direct) {
                fprintf(dest, "%d", address.displacement);
            } else if (address.displacement != 0) {
                fprintf(dest, "%+d", address.displacement);
            }
            fprintf(dest, "]");
            break;
        }
        case OperandImmediate: {
            fprintf(dest, "%d", operand.immediate);
            break;
        }
        case OperandRelativeImmediate: {
            fprintf(dest, "%+d", operand.s_immediate);
            break;
        }
        default:
            fprintf(stderr, "ERROR: unknown operand encountered.\n");
            exit(1);
    };
}

void print_instruction(Instruction* inst, FILE *dest) {
    fprintf(dest, "%s ", get_mnemonic(inst->op));
    print_operand(inst, 0, dest);
    if (inst->operands[1].kind != OperandNone) {
        fprintf(dest, ", ");
        print_operand(inst, 1, dest);
    }
}

void print_registers(u16 reg_state[], u32 ip) {
    for (int i = 1; i < Reg_count; i++) {
        printf("%8s: 0x%04x (%u)\n", get_reg_name(i, 2), reg_state[i], reg_state[i]);
    }
    printf("%8s: 0x%04x (%u)\n", "ip", ip, ip);
}
