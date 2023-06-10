#include "sim8086.h"
#include "sim8086_mnemonics.h"


char const *OperationMnemonics[]
{
    "mov",
    "add",
    "sub",
    "cmp",
    "jne",
    "je",
    "jl",
    "jle",
    "jb",
    "jbe",
    "jp",
    "jo",
    "js",
    "jnl",
    "jg",
    "jnb",
    "ja",
    "jnp",
    "jno",
    "jns",
    "LOOP",
    "LOOPZ",
    "LOOPNZ",
    "JCXZ",
    "ret",
    "unknown",
};


char const *RegisterMnemonics[]
{
    "al",
    "cl",
    "dl",
    "bl",
    "ah",
    "ch",
    "dh",
    "bh",
    "ax",
    "cx",
    "dx",
    "bx",
    "sp",
    "bp",
    "si",
    "di",
    "bx + si",
    "bx + di",
    "bp + si",
    "bp + di",
    "unknown",
};


char const *GetOpMnemonic(operation_types op)
{
    char const *Result = "";

    if(op < Op_count)
    {
        Result = OperationMnemonics[op];
    }

    return Result;
}


char const *GetRegisterMnemonic(register_id regMemId)
{
    char const *Result = "";

    if(regMemId < Reg_mem_id_count)
    {
        Result = RegisterMnemonics[regMemId];
    }

    return Result;
}
