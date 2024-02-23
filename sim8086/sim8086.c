#include <string.h>
#include <stdbool.h>
#include "sim8086.h"
#include "sim8086_print.h"

void disassemble(u8 buffer[], u32 n);
void run(u8 buffer[], u32 n);
Instruction decode(u8 buffer[], u32 idx);
void execute_instruction(Instruction* inst, u16 reg_state[]);
void decode_rm_reg(Instruction* inst, const u8 buffer[]);
void set_reg_operand(Instruction* inst, u8 reg, u8 wide, u8 operand_num);
void set_effective_address_operand(Instruction* inst, const u8 buffer[], u8 rm, u8 mod, u8 operand_num);
void set_immediate_operand(Instruction* inst, const u8 buffer[], u8 sign, u8 wide, u8 operand_num);
void decode_im_to_reg(Instruction* inst, const u8 buffer[]);
void decode_im_to_rm(Instruction* inst, const u8 buffer[]);
void decode_acc_mem(Instruction* inst, const u8 buffer[]);
void decode_im_to_acc(Instruction* inst, const u8 buffer[]);
void decode_jmp(Instruction* inst, const u8 buffer[]);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s [8086 machine code file] ...\n", argv[0]);
        exit(1);
    }

    bool execute = false;
    u8 buffer[BUFFER_SIZE];

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-exec") == 0) {
            execute = true;
        } else {
            FILE *fp;
            if ((fp = fopen(argv[i], "rb")) == NULL) {
                fprintf(stderr, "ERROR: unable to open %s\n", argv[i]);
                exit(1);
            };
            u32 bytes_read = fread(buffer, sizeof(u8), BUFFER_SIZE, fp);
            fclose(fp);
            if (execute) {
                run(buffer, bytes_read);
            } else {
                disassemble(buffer, bytes_read);
            }
        }
    }
    return EXIT_SUCCESS;
}

void run(u8 buffer[], u32 n) {
    u32 i = 0;
    u16 reg_state[Reg_count] = { 0 };
    while (i < n) {
        Instruction inst = decode(buffer, i);
        if (i + inst.size <= n) {
            i += inst.size;
        } else {
            fprintf(stderr, "ERROR: instruction exceeds disassembly region.\n");
            exit(1);
        }
        print_instruction(&inst, stdout);
        printf(" ; ");
        execute_instruction(&inst, reg_state);
        printf("\n");
    }
    printf("\n");
    printf("Final registers:\n");
    print_registers(reg_state);
}

void execute_instruction(Instruction* inst, u16 reg_state[]) {
//    OpType op_type = inst->op;

    Operand* dest_op = &inst->operands[0];
    Operand* src_op = &inst->operands[1];

//    u32 wide = inst->flags & FlagWide;

    // determine destination address
    u16 *dest;
    if (dest_op->kind == OperandRegister) {
        RegisterAccess reg = dest_op->reg;
        dest = (u16*)((u8*)&reg_state[reg.index] + reg.offset);
    } else {
        assert(false); // shouldn't be here yet
    }

    // determine src
    u16 src;
    if (src_op->kind == OperandImmediate) {
        src = (u16)src_op->immediate;
    } else if (src_op->kind == OperandRegister) {
        src = reg_state[src_op->reg.index];
    } else {
        src = 0;
    }

    printf("0x%x->", *dest);
    *dest = src;
    printf("0x%x", *dest);
}

void disassemble(u8 buffer[], u32 n) {
    u32 i = 0;
    while (i < n) {
        Instruction inst = decode(buffer, i);
        if (i + inst.size <= n) {
            i += inst.size;
        } else {
            fprintf(stderr, "ERROR: instruction exceeds disassembly region.\n");
            exit(1);
        }
        print_instruction(&inst, stdout);
        printf("\n");
    }
}

Instruction decode(u8 buffer[], u32 idx) {
    Instruction inst;
    inst.address = idx;
    inst.op = OpNone;
    inst.flags = 0;

    u8 opcode = buffer[inst.address];

    // check for jump opcodes
    switch (opcode) {
        case 0b01110100:
            inst.op = OpJe;
            break;
        case 0b01111100:
            inst.op = OpJl;
            break;
        case 0b01111110:
            inst.op = OpJle;
            break;
        case 0b01110010:
            inst.op = OpJb;
            break;
        case 0b01110110:
            inst.op = OpJbe;
            break;
        case 0b01111010:
            inst.op = OpJp;
            break;
        case 0b01110000:
            inst.op = OpJo;
            break;
        case 0b01111000:
            inst.op = OpJs;
            break;
        case 0b01110101:
            inst.op = OpJne;
            break;
        case 0b01111101:
            inst.op = OpJnl;
            break;
        case 0b01111111:
            inst.op = OpJnle;
            break;
        case 0b01110011:
            inst.op = OpJnb;
            break;
        case 0b01110111:
            inst.op = OpJnbe;
            break;
        case 0b01111011:
            inst.op = OpJnp;
            break;
        case 0b01110001:
            inst.op = OpJno;
            break;
        case 0b01111001:
            inst.op = OpJns;
            break;
        case 0b11100010:
            inst.op = OpLoop;
            break;
        case 0b11100001:
            inst.op = OpLoopz;
            break;
        case 0b11100000:
            inst.op = OpLoopnz;
            break;
        case 0b11100011:
            inst.op = OpJcxz;
            break;
    }

    if (inst.op != OpNone) { // must have matched a jmp code
        decode_jmp(&inst, buffer);
        return inst;
    }

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

void decode_jmp(Instruction* inst, const u8 buffer[]) {
    Operand signed_immediate;
    signed_immediate.kind = OperandRelativeImmediate;
    signed_immediate.s_immediate = (i32)(i8)buffer[inst->address + 1];
    inst->operands[0] = signed_immediate;
    Operand none;
    none.kind = OperandNone;
    inst->operands[1] = none;
    inst->size = 2;
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
    if (wide) {
        inst->flags |= FlagWide;
    }
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
