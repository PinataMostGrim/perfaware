#ifndef SIM8086_MNEMONICS_H
#define SIM8086_MNEMONICS_H

#include "base.h"
#include "memory_arena.h"
#include "sim8086.h"

global_function char const *GetOpMnemonic(operation_types op);
global_function char const *GetRegisterMnemonic(register_id regMemId);
global_function const char *GetRegisterFlagMnemonic(register_flags flag);
global_function char *GetInstructionMnemonic(instruction *instruction, memory_arena *arena);
global_function char *GetInstructionBitsMnemonic(instruction inst, memory_arena *arena);

#endif // SIM8086_MNEMONICS_H
