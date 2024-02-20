#include "sim8086.h"
#include "sim8086_print.h"

void disassemble(u8 buffer[], u32 n);
Instruction get_next_instruction(u8 buffer[], u32 idx);
void decode_rm_reg(Instruction* inst, const u8 buffer[]);
void set_reg_operand(Instruction* inst, u8 reg, u8 wide, u8 operand_num);
void set_effective_address_operand(Instruction* inst, const u8 buffer[], u8 rm, u8 mod, u8 operand_num);
void set_immediate_operand(Instruction* inst, const u8 buffer[], u8 sign, u8 wide, u8 operand_num);
void decode_im_to_reg(Instruction* inst, const u8 buffer[]);
void decode_im_to_rm(Instruction* inst, const u8 buffer[]);
void decode_acc_mem(Instruction* inst, const u8 buffer[]);
void decode_im_to_acc(Instruction* inst, const u8 buffer[]);

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
    inst.flags = 0;

    u8 opcode = buffer[inst.address];

    // opcodes of len 7
    opcode >>= 1;
    switch (opcode) {
        case 0b1100011:
            inst.op = OpMov;
            decode_im_to_rm(&inst, buffer);
            return inst;
        case 0b1010000:
        case 0b1010001:
            inst.op = OpMov;
            decode_acc_mem(&inst, buffer);
            return inst;
        case 0b0000010:
            inst.op = OpAdd;
            decode_im_to_acc(&inst, buffer);
            return inst;
        case 0b0010110:
            inst.op = OpSub;
            decode_im_to_acc(&inst, buffer);
            return inst;
        case 0b0011110:
            inst.op = OpCmp;
            decode_im_to_acc(&inst, buffer);
            return inst;
    }

    // opcodes of len 6
    opcode >>= 1;
    switch (opcode) {
        case 0b100010:
            inst.op = OpMov;
            decode_rm_reg(&inst, buffer);
            return inst;
        case 0b000000:
            inst.op = OpAdd;
            decode_rm_reg(&inst, buffer);
            return inst;
        case 0b001010:
            inst.op = OpSub;
            decode_rm_reg(&inst, buffer);
            return inst;
        case 0b001110:
            inst.op = OpCmp;
            decode_rm_reg(&inst, buffer);
            return inst;
        case 0b100000:
            decode_im_to_rm(&inst, buffer);
            return inst;
    }

    // opcodes of len 4
    opcode >>= 2;
    switch (opcode) {
        case 0b1011:
            inst.op = OpMov;
            decode_im_to_reg(&inst, buffer);
            return inst;
    }

    if (inst.op == OpNone) {
        fprintf(stderr, "ERROR: unknown opcode encountered.\n");
        exit(1);
    }

    return inst;
}

void decode_im_to_acc(Instruction* inst, const u8 buffer[]) {
    u32 idx = inst->address;
    u8 w = buffer[idx] & 1;
    inst->size = 1;
    set_reg_operand(inst, 0, w, 0);
    set_immediate_operand(inst, buffer, 0, w, 1);
}

void decode_acc_mem(Instruction* inst, const u8 buffer[]) {
    u32 idx = inst->address;
    u8 to_memory = (buffer[idx] >> 1) & 1;
    u8 w = buffer[idx] & 1;
    inst->size = 1;
    set_reg_operand(inst, 0, w, to_memory);
    set_effective_address_operand(inst, buffer, 0b110, 0b00, to_memory ? 0 : 1);
}

void decode_im_to_rm(Instruction* inst, const u8 buffer[]) {
    u32 idx = inst->address;
    u8 s = (buffer[idx] >> 1) & 1;
    u8 w = buffer[idx] & 1;
    idx++;
    u8 mod = buffer[idx] >> 6;
    u8 op_type = (buffer[idx] >> 3) & 0b111;
    u8 rm = buffer[idx] & 0b111;

    if (inst->op != OpMov && op_type == 0b000) {
        inst->op = OpAdd;
    } else if (op_type == 0b101) {
        inst->op = OpSub;
    } else if (op_type == 0b111) {
        inst->op = OpCmp;
    }

    inst->size = 2;
    if (mod == 0b11) {
        set_reg_operand(inst, rm, w, 0);
    } else {
        set_effective_address_operand(inst, buffer, rm, mod, 0);
    }
    set_immediate_operand(inst, buffer, inst->op == OpMov ? 0 : s, w, 1);
}

void decode_im_to_reg(Instruction* inst, const u8 buffer[]) {
    u32 idx = inst->address;
    u8 w = (buffer[idx] >> 3) & 1;
    u8 reg = buffer[idx] & 0b111;
    set_reg_operand(inst, reg, w, 0);
    inst->size = 1;
    set_immediate_operand(inst, buffer, 0, w, 1);
}

void decode_rm_reg(Instruction* inst, const u8 buffer[]) {
    u32 idx = inst->address;
    u8 d = (buffer[idx] >> 1) & 1;
    u8 w = buffer[idx] & 1;
    idx++;
    u8 mod = buffer[idx] >> 6;
    u8 reg = (buffer[idx] >> 3) & 0b111;
    u8 rm = buffer[idx] & 0b111;
    inst->size = 2;
    if (mod == 0b11) {
        set_reg_operand(inst, rm, w, d);
    } else {
        set_effective_address_operand(inst, buffer, rm, mod, d);
    }
    set_reg_operand(inst, reg, w, d ? 0 : 1);
}

void set_reg_operand(Instruction* inst, u8 reg, u8 wide, u8 operand_num) {
    const RegisterAccess reg_table[][2] = {
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
    inst->operands[operand_num] = res;
}

void set_effective_address_operand(Instruction* inst, const u8 buffer[], u8 rm, u8 mod, u8 operand_num) {
    Operand res;
    res.kind = OperandMemory;
    if (mod == 0b00 && rm == 0b110) {
        res.address.base = Ea_direct;
    } else {
        res.address.base = (EffectiveAddressBase) (rm + 1);
    }
    res.address.displacement = 0;
    u32 idx = inst->address + inst->size - 1;
    if (mod == 0b10 || (mod == 0b00 && rm == 0b110)) {
        idx += 2;
        res.address.displacement = (i16)(buffer[idx] << 8) | buffer[idx-1]; // parse as signed
        inst->flags |= FlagWide;
        inst->size += 2;
    } else if (mod == 0b01) {
        idx += 1;
        res.address.displacement = (i16)(buffer[idx] << 8) >> 8; // sign extend to 16 bits
        inst->size += 1;
    }
    inst->operands[operand_num] = res;
}

void set_immediate_operand(Instruction* inst, const u8 buffer[], u8 sign, u8 wide, u8 operand_num) {
    Operand res;
    res.kind = OperandImmediate;
    u32 idx = inst->address + inst->size - 1;
    if (sign == 0 && wide == 1) {
        idx += 2;
        res.immediate = (u32)((buffer[idx] << 8) | buffer[idx-1]);
        inst->size += 2;
    } else {
        idx += 1;
        if (sign == 1 && wide == 1) {
            res.immediate = (i16)(buffer[idx] << 8) >> 8; // sign extend to 16 bits
        } else {
            res.immediate = (u32)(buffer[idx]);
        }
        inst->size += 1;
    }
    inst->operands[operand_num] = res;
}
