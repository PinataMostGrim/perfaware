#ifndef SIM8086_MNEMONICS_H
#define SIM8086_MNEMONICS_H

#include "sim8086.h"

char const *GetOpMnemonic(operation_types op);
char const *GetRegisterMnemonic(register_id regMemId);
#endif
