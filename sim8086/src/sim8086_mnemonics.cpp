#include <stdio.h>

#include "base_memory.h"
#include "base_string.h"
#include "sim8086.h"


global_variable char const *OperationMnemonics[]
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


global_variable char const *RegisterMnemonics[]
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


// TODO (Aaron): Figure out how to assert that this matches the number of elements in 'register_flags'
// Normally I would add a 'RegisterFlags_count' element, but it's being used as a flag so that won't work.
global_variable char const *RegisterFlagMnemonics[]
{
    "CF",
    "PF",
    "AF",
    "ZF",
    "SF",
    "OF",
};


global_function char const *GetOpMnemonic(operation_types op)
{
    char const *Result = "";

    if(op < Op_count)
    {
        Result = OperationMnemonics[op];
    }

    return Result;
}


global_function char const *GetRegisterMnemonic(register_id regMemId)
{
    char const *Result = "";

    if(regMemId < Reg_mem_id_count)
    {
        Result = RegisterMnemonics[regMemId];
    }

    return Result;
}


global_function const char *GetRegisterFlagMnemonic(register_flags flag)
{
    switch (flag)
    {
        case RegisterFlag_CF:
            return RegisterFlagMnemonics[0];
        case RegisterFlag_PF:
            return RegisterFlagMnemonics[1];
        case RegisterFlag_AF:
            return RegisterFlagMnemonics[2];
        case RegisterFlag_ZF:
            return RegisterFlagMnemonics[3];
        case RegisterFlag_SF:
            return RegisterFlagMnemonics[4];
        case RegisterFlag_OF:
            return RegisterFlagMnemonics[5];
        default:
            Assert(FALSE && "Unhandled register flag enum");
            return "";
    }
}


global_function Str8 GetInstructionMnemonic(instruction *instruction, memory_arena *arena)
{
    U8 *startPtr = (U8 *)ArenaPushCStringf(arena, FALSE, (char *)"%s ", GetOpMnemonic(instruction->OpType));
    const char *Separator = "";

    for (int i = 0; i < ArrayCount(instruction->Operands); ++i)
    {
        instruction_operand operand = instruction->Operands[i];

        // skip empty operands
        if(operand.Type == Operand_None)
        {
            continue;
        }

        if (GetStringLength((char *)Separator) > 0)
        {
            ArenaPushCStringf(arena, FALSE, (char *)"%s", Separator);
        }
        Separator = ", ";

        switch (operand.Type)
        {
            case Operand_None:
            {
                break;
            }

            case Operand_Memory:
            {
                // prepend width hint if necessary
                if (operand.Memory.Flags & Memory_PrependWidth)
                {
                    if (operand.Memory.Flags & Memory_IsWide)
                    {
                        ArenaPushCStringf(arena, FALSE, (char *)"%s", (char *)"word ");
                    }
                    else
                    {
                        ArenaPushCStringf(arena, FALSE, (char *)"%s", "byte ");
                    }

                }

                // print direct address
                if (operand.Memory.Flags & Memory_HasDirectAddress)
                {
                    ArenaPushCStringf(arena, FALSE, (char *)"[%i]", operand.Memory.DirectAddress);
                    break;
                }

                // print memory with optional displacement
                ArenaPushCStringf(arena, FALSE, (char *)"[");
                ArenaPushCStringf(arena, FALSE, (char *)GetRegisterMnemonic(operand.Memory.Register));

                if (operand.Memory.Flags & Memory_HasDisplacement)
                {
                    if (operand.Memory.Displacement >= 0)
                    {
                        ArenaPushCStringf(arena, FALSE, (char *)" + %i", operand.Memory.Displacement);
                    }
                    else
                    {
                        ArenaPushCStringf(arena, FALSE, (char *)" - %i", operand.Memory.Displacement);
                    }
                }

                ArenaPushCStringf(arena, FALSE, (char *)"]");
                break;
            }

            case Operand_Register:
            {
                ArenaPushCStringf(arena, FALSE, (char *)GetRegisterMnemonic(operand.Register));
                break;
            }

            case Operand_Immediate:
            {
                if (operand.Immediate.Flags & Immediate_IsJump)
                {
                    S8 offset = (S8)(operand.Immediate.Value & 0xff);

                    // Note (Aaron): Offset the value to accommodate a NASM syntax peculiarity.
                    // NASM expects an offset value from the start of the instruction rather than
                    // the end (which is how the instructions are encoded).
                    offset += instruction->Bits.ByteCount;

                    if (offset >= 0)
                    {
                        ArenaPushCStringf(arena, FALSE, (char *)"$+%i", offset);
                    }
                    else
                    {
                        ArenaPushCStringf(arena, FALSE, (char *)"$%i", offset);
                    }
                    break;
                }

                // TODO (Aaron): Test this more
                bool isSigned = operand.Immediate.Flags & Immediate_IsSigned;
                ArenaPushCStringf(arena, FALSE, (char *)"%i",
                             (isSigned
                                 ? (S16) operand.Immediate.Value
                                 : (U16) operand.Immediate.Value));
                break;
            }

            default:
            {
                ArenaPushCStringf(arena, FALSE, (char *)"?");
            }
        }
    }

    U64 length = arena->PositionPtr - startPtr;
    Str8 result = String8(startPtr, length);

    // Note (Aaron): Append the null-terminator character
    ArenaPushSizeZero(arena, 1);

    return result;
}


global_function Str8 GetInstructionBitsMnemonic(instruction *inst, memory_arena *arena)
{
    U8 *startPtr = arena->PositionPtr;
    for (U8 i = 0; i < inst->Bits.ByteCount; ++i)
    {
        for (U8 j = 0; j < 8; j++)
        {
            U8 mask = 128 >> j;
            U8 bit = (inst->Bits.Bytes[i] & mask) >> (7 - j);
            ArenaPushCString(arena, FALSE, bit == 1 ? (char *)"1": (char *)"0");
        }
    }

    // Note (Aaron): Append the null-terminator character
    ArenaPushSizeZero(arena, 1);

    U64 length = arena->PositionPtr - startPtr;
    Str8 result = String8(startPtr, length);

    return result;
}
