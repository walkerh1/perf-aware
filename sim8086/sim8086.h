#ifndef PERF_AWARE_SIM8086_H
#define PERF_AWARE_SIM8086_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define len(arr) (sizeof(arr) / sizeof(arr[0]))

#define BUFFER_SIZE (1024*1024)

typedef enum {
    FlagWide = (1 << 0),
} FlagTypes;

typedef enum {
    OpNone,
    OpMov,
    OpAdd,
    OpSub,
    OpCmp,
    OpJe,
    OpJl,
    OpJle,
    OpJb,
    OpJbe,
    OpJp,
    OpJo,
    OpJs,
    OpJne,
    OpJnl,
    OpJnle,
    OpJnb,
    OpJnbe,
    OpJnp,
    OpJno,
    OpJns,
    OpLoop,
    OpLoopz,
    OpLoopnz,
    OpJcxz,
    OpCount,
} OpType;

typedef enum {
    Reg_none,
    Reg_a,
    Reg_b,
    Reg_c,
    Reg_d,
    Reg_sp,
    Reg_bp,
    Reg_si,
    Reg_di,
    Reg_count,
} Register;

typedef enum {
    Ea_direct,
    Ea_bx_si,
    Ea_bx_di,
    Ea_bp_si,
    Ea_bp_di,
    Ea_si,
    Ea_di,
    Ea_bp,
    Ea_bx,
    Ea_count,
} EffectiveAddressBase;

typedef struct {
    Register segment;
    EffectiveAddressBase base;
    i32 displacement;
} EffectiveAddress;

typedef struct {
    Register index;
    u8 offset;
    u8 count;
} RegisterAccess;

typedef enum {
    OperandNone,
    OperandRegister,
    OperandMemory,
    OperandImmediate,
    OperandRelativeImmediate,
} OperandType;

typedef struct {
    OperandType kind;
    union {
        RegisterAccess reg;
        EffectiveAddress address;
        u32 immediate;
        i32 s_immediate;
    };
} Operand;

typedef struct {
    u32 address;            // relative address in buffer
    u32 size;               // size of instruction in bytes
    OpType op;              // opcode type
    Operand operands[2];    // instruction operands
    u32 flags;              // flags
} Instruction;

typedef enum {
    Zero_flag = (1 << 0),
    Sign_flag = (1 << 1),
    Flag_count,
} Flag;

#endif