#ifndef PERF_AWARE_SIM8086_PRINT_H
#define PERF_AWARE_SIM8086_PRINT_H

void print_instruction(Instruction* inst, FILE *dest);
void print_registers(u16 reg_state[]);

#endif
