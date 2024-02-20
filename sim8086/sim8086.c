#include "sim8086.h"

void disassemble(u8 buffer[], u32 n);
Instruction get_next_instruction(u8 buffer[], u32 idx);
void print_instruction(Instruction* inst, FILE* dest);
void decode_rm_reg(Instruction* inst, const u8 buffer[]);
Operand get_reg_operand(u8 register_idx, u8 wide);
void print_operand(Operand operand, FILE* dest);
Operand get_effective_address(u8 rm_idx, u8 mod);

int main(int argc, char *argv[]) {
    // make sure at least one machine code file was provided
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s [8086 machine code file] ...\n", argv[0]);
        exit(1);
    }

    u8 buffer[BUFFER_SIZE]; // buffer for holding raw bytes of 8086 machine code

    for (int i = 1; i < argc; i++) {
        FILE *fp;
        if ((fp = fopen(argv[1], "rb")) == NULL) {
            fprintf(stderr, "ERROR: unable to open %s\n", argv[1]);
            exit(1);
        };

        u32 bytes_read = fread(buffer, sizeof(u8), BUFFER_SIZE, fp);
        fclose(fp);

        disassemble(buffer, bytes_read);
    }

    return EXIT_SUCCESS;
}

void disassemble(u8 buffer[], u32 n) {
    u32 i = 0;
    while (i < n) {
        Instruction inst = get_next_instruction(buffer, i);
        if (i + inst.size <= n) {
            i += inst.size;
        } else {
            fprintf(stderr, "ERROR: instruction exceeds disassembly region.\n");
            exit(1);
        }
        print_instruction(&inst, stdout);
    }
}

Instruction get_next_instruction(u8 buffer[], u32 idx) {
    Instruction inst;
    inst.address = idx;

    u8 opcode = buffer[inst.address];
    opcode >>= 2;
    switch (opcode) {
        case 0b100010:
            inst.op = OpMov;
            decode_rm_reg(&inst, buffer);
            break;
        default:
            fprintf(stderr, "ERROR: unknown opcode encountered.\n");
            exit(1);
    }

    return inst;
}

void decode_rm_reg(Instruction* inst, const u8 buffer[]) {
    u32 idx = inst->address;
    u8 d, w, mod, reg, rm;
    d = (buffer[idx] >> 1) & 1;
    w = buffer[idx] & 1;
    idx++;
    mod = buffer[idx] >> 6;
    reg = (buffer[idx] >> 3) & 0b111;
    rm = buffer[idx] & 0b111;
    if (mod == 0b11) {
        inst->operands[d] = get_reg_operand(rm, w);
    } else {
        Operand operand = get_effective_address(rm, mod);
        if (mod == 0b10 || (mod == 0b00 && rm == 0b011)) {
            idx += 2;
            operand.address.displacement = (buffer[idx] << 8) | buffer[idx-1];
        } else if (mod == 0b01) {
            idx += 1;
            operand.address.displacement = buffer[idx];
        }
        inst->operands[d] = operand;
    }
    inst->operands[d ? 0 : 1] = get_reg_operand(reg, w);
    inst->size = idx - inst->address + 1;
}

Operand get_effective_address(u8 rm, u8 mod) {
    Operand res;
    res.kind = OperandMemory;
    if (mod == 0b00 && rm == 0b110) {
        res.address.base = Ea_direct;
    } else {
        res.address.base = (EffectiveAddressBase) (rm + 1);
    }
    return res;
}

Operand get_reg_operand(u8 reg, u8 wide) {
    RegisterAccess reg_table[][2] = {
        {{Reg_a, 0, 1}, {Reg_a, 0, 2}},
        {{Reg_c, 0, 1}, {Reg_c, 0, 2}},
        {{Reg_d, 0, 1}, {Reg_d, 0, 2}},
        {{Reg_b, 0, 1}, {Reg_b, 0, 2}},
        {{Reg_a, 1, 1}, {Reg_sp, 0, 2}},
        {{Reg_c, 1, 1}, {Reg_bp, 0, 2}},
        {{Reg_d, 1, 1}, {Reg_si, 0, 2}},
        {{Reg_b, 1, 1}, {Reg_di, 0, 2}},
    };
    Operand res;
    res.kind = OperandRegister;
    res.reg = reg_table[reg][wide];
    return res;
}

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

void print_instruction(Instruction* inst, FILE *dest) {
    fprintf(dest, "%s ", get_mnemonic(inst->op));
    print_operand(inst->operands[0], dest);
    fprintf(dest, ", ");
    print_operand(inst->operands[1], dest);
    fprintf(dest, "\n");
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
        default:
            fprintf(stderr, "ERROR: unknown operand encountered.\n");
            exit(1);
    };
}