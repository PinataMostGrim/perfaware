#ifndef SIM8086_H
#define SIM8086_H

#include <stdint.h>

#include "sim8086_mnemonics.h"

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;


#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))


struct sim_memory
{
    uint8 *Memory;
    uint8 *ReadPtr;
    uint16 Size = 0;
    uint16 MaxSize = 0;
};


enum operand_types
{
    Operand_none,
    Operand_register,
    Operand_memory,
    Operand_immediate,
};


enum operand_memory_flags : uint8
{
    Memory_HasDisplacement = 0x1,
    Memory_DirectAddress = 0x2,
    Memory_PrependWidth = 0x4,
    Memory_IsWide = 0x8,
};


struct operand_memory
{
    register_id Register;
    int32 Displacement;
    uint16 DirectAddress;
    uint32 Flags;
};


struct instruction_operand
{
    operand_types Type = Operand_none;
    union
    {
        operand_memory Memory;
        register_id Register;
        int32 ImmediateValue;
    };
};


struct instruction
{
    operation_types OpType = Op_unknown;
    instruction_operand Operands[2] = {};

    uint8 DirectionBit = 0;
    uint8 WidthBit = 0;
    uint8 ModBits = 0;
    uint8 RegBits = 0;
    uint8 RmBits = 0;
    uint8 SignBit = 0;
};


struct decode_context
{
    uint32 InstructionCount = 0;
    union
    {
        uint8 buffer[6] = {};
        struct
        {
            uint8 byte0;
            uint8 byte1;
            uint8 byte2;
            uint8 byte3;
            uint8 byte4;
            uint8 byte5;
        };
    };

    uint8 *bufferPtr = buffer;
};

#endif
