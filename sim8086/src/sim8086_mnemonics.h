#ifndef SIM8086_MNEMONICS_H
#define SIM8086_MNEMONICS_H

#include "memory_arena.h"
#include "sim8086.h"

static char const *GetOpMnemonic(operation_types op);
static char const *GetRegisterMnemonic(register_id regMemId);
static const char *GetRegisterFlagMnemonic(register_flags flag);
static char *GetInstructionMnemonic(instruction *instruction, memory_arena *arena);

#endif // SIM8086_MNEMONICS_H
