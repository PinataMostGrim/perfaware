#ifndef SIM8086_H
#define SIM8086_H

#include <stdint.h>

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


enum register_id
{
    Reg_unknown,
    Reg_al,
    Reg_cl,
    Reg_dl,
    Reg_bl,
    Reg_ah,
    Reg_ch,
    Reg_dh,
    Reg_bh,
    Reg_ax,
    Reg_cx,
    Reg_dx,
    Reg_bx,
    Reg_sp,
    Reg_bp,
    Reg_si,
    Reg_di,
    Reg_bx_si,
    Reg_bx_di,
    Reg_bp_si,
    Reg_bp_di,

    Reg_mem_id_count,
};


struct register_info
{
    register_id Register = Reg_unknown;
    uint8 RegisterIndex = 0;
    bool IsWide = true;
    uint16 Mask = 0x0;
};


enum register_flags : uint8
{
    Register_CF = 0x1,
    Register_PF = 0x2,
    Register_AF = 0x4,
    Register_ZF = 0x8,      // Did an arithmetic operation produce a value of 0?
    Register_SF = 0x10,     // Did an arithmetic operation produce a negative value?
    Register_OF = 0x20,
};


enum operation_types
{
    Op_unknown,
    Op_mov,
    Op_add,
    Op_sub,
    Op_cmp,
    Op_jmp,

    Op_count,
};


enum operand_types
{
    Operand_None,
    Operand_Register,
    Operand_Memory,
    Operand_Immediate,
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
    operand_types Type = Operand_None;
    union
    {
        operand_memory Memory;
        register_id Register;
        uint16 ImmediateValue;
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
    size_t InstructionCount = 0;
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
