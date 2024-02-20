#include "sim8086.h"
#include "sim8086_print.h"

char* get_mnemonic(u32 idx) {
    char *mnemonics[] = {
            "",
            "mov",
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
            "bx + si",
            "bx + di",
            "bp + si",
            "bp + di",
            "si",
            "di",
            "bp",
            "bx",
    };
    return rm_base[address.base];
}

void print_operand(Operand operand, FILE* dest) {
    OperandType type = operand.kind;
    switch (type) {
        case OperandRegister: {
            RegisterAccess reg = operand.reg;
            fprintf(dest, "%s", get_reg_name(reg.index, (reg.count == 2) ? 2 : reg.offset));
            break;
        }
        case OperandMemory: {
            EffectiveAddress address = operand.address;
            fprintf(dest, "[%s", get_effective_address_base_name(address));
            if (address.displacement != 0) {
                fprintf(dest, " + %d", address.displacement);
            }
            fprintf(dest, "]");
            break;
        }
        case OperandImmediate: {
            fprintf(dest, "%d", operand.immediate);
            break;
        }
        default:
            fprintf(stderr, "ERROR: unknown operand encountered.\n");
            exit(1);
    };
}

void print_instruction(Instruction* inst, FILE *dest) {
    fprintf(dest, "%s ", get_mnemonic(inst->op));
    print_operand(inst->operands[0], dest);
    fprintf(dest, ", ");
    print_operand(inst->operands[1], dest);
    fprintf(dest, "\n");
}
