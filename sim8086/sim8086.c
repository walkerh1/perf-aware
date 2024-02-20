#include "sim8086.h"
#include "sim8086_print.h"

void disassemble(u8 buffer[], u32 n);
Instruction get_next_instruction(u8 buffer[], u32 idx);
void decode_rm_reg(Instruction* inst, const u8 buffer[]);
Operand get_reg_operand(u8 register_idx, u8 wide);
Operand get_effective_address(u8 rm_idx, u8 mod);
void decode_im_to_reg(Instruction* inst, const u8 buffer[]);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s [8086 machine code file] ...\n", argv[0]);
        exit(1);
    }

    u8 buffer[BUFFER_SIZE];

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
    inst.op = OpNone;

    u8 opcode = buffer[inst.address];
    opcode >>= 2;
    switch (opcode) {
        case 0b100010:
            inst.op = OpMov;
            decode_rm_reg(&inst, buffer);
            break;
    }

    opcode >>= 2;
    switch (opcode) {
        case 0b1011:
            inst.op = OpMov;
            decode_im_to_reg(&inst, buffer);
            break;
    }

    if (inst.op == OpNone) {
        fprintf(stderr, "ERROR: unknown opcode encountered.\n");
        exit(1);
    }

    return inst;
}

void decode_im_to_reg(Instruction* inst, const u8 buffer[]) {
    u32 idx = inst->address;
    u8 w = (buffer[idx] >> 3) & 1;
    u8 reg = buffer[idx] & 0b111;

}

void decode_rm_reg(Instruction* inst, const u8 buffer[]) {
    u32 idx = inst->address;
    u8 d = (buffer[idx] >> 1) & 1;
    u8 w = buffer[idx] & 1;
    idx++;
    u8 mod = buffer[idx] >> 6;
    u8 reg = (buffer[idx] >> 3) & 0b111;
    u8 rm = buffer[idx] & 0b111;
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
