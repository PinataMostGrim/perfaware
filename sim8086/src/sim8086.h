#ifndef SIM8086_H
#define SIM8086_H

#include <stdint.h>
#include <assert.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;


#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#if SIM8086_SLOW
#define assert_8086(expression) assert(expression)
#define static_assert_8086(expression, string) static_assert(expression, string)
#else
#define assert_8086(expression)
#define static_assert_8086(expression, string)
#endif


enum register_id
{
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
    Reg_unknown,

    Reg_mem_id_count,
};


struct register_info
{
    register_id Register = Reg_unknown;
    uint8 RegisterIndex = 0;
    bool IsWide = true;
    uint16 Mask = 0x0;
};


// Flags:
// OF | SF | ZF | AF | PF | CF
//     CF - Carry flag
//     PF - Parity flag
//     AF - Auxiliary Carry flag
//     ZF - Zero flag
//     SF - Sign flag
//     OF - Overflow flag
enum register_flags : uint8
{
    Register_CF = 0x1,
    Register_PF = 0x2,
    Register_AF = 0x4,
    Register_ZF = 0x8,          // Did an arithmetic operation produce a value of 0?
    Register_SF = 0x10,         // Did an arithmetic operation produce a negative value?
    Register_OF = 0x20,
};


struct processor_8086
{
    union
    {
        uint16 Registers[8] = {};   // Note (Aaron): Registers are: A, B, C, D, SP, BP, SI and DI
        struct
        {
            uint16 RegisterAX;
            uint16 RegisterBX;
            uint16 RegisterCX;
            uint16 RegisterDX;
            uint16 RegisterSP;
            uint16 RegisterBP;
            uint16 RegisterSI;
            uint16 RegisterDI;
        };
    };
    uint8 Flags = 0;            // Note (Aaron): Register flags
    uint32 IP = 0;              // Note (Aaron): Instruction pointer
    uint32 PrevIP = 0;          // Note (Aaron): Previous instruction pointer

    // Note (Aaron): The 8086 had 16 memory segments of 64k bytes each. Memory segments could
    // be slid around like windows over the entire memory surface, however I'm not replicating
    // that here. I'm arbitrarily allocating 1 memory segment (64k bytes) for program memory
    // and the remaining 15 memory segments for the program to make use of.
    uint32 MemorySize = 1024 * 1024;
    uint8 *Memory;
    uint32 ProgramSize = 0;

    // Note (Aaron): Number of instructions decoded from the loaded program
    uint32 InstructionCount = 0;

    // Note (Aaron): Number of clock cycles used by the loaded program
    uint32 TotalClockCount = 0;
};


// TODO (Aaron): Define underlying type? uint32_t?
// TODO (Aaron): Move Op_unknown to position 0 so that it is the default value (initialize to 0)
enum operation_types
{
    Op_mov,
    Op_add,
    Op_sub,
    Op_cmp,
    Op_jne,
    Op_je,
    Op_jl,
    Op_jle,
    Op_jb,
    Op_jbe,
    Op_jp,
    Op_jo,
    Op_js,
    Op_jnl,
    Op_jg,
    Op_jnb,
    Op_ja,
    Op_jnp,
    Op_jno,
    Op_jns,
    Op_loop,
    Op_loopz,
    Op_loopnz,
    Op_jcxz,
    Op_unknown,
    Op_ret,

    Op_count,
};


enum operand_types
{
    Operand_None,
    Operand_Register,
    Operand_Memory,
    Operand_Immediate,
};


struct operand_memory
{
    register_id Register;
    int16 Displacement;
    uint16 DirectAddress;
    uint8 Flags;
};


enum operand_memory_flags : uint8
{
    Memory_HasDisplacement = 0x1,
    Memory_HasDirectAddress = 0x2,
    // TODO (Aaron): This may not be necessary if we should ALWAYS prepend the width
    Memory_PrependWidth = 0x4,
    Memory_IsWide = 0x8,
};


struct operand_immediate
{
    uint16 Value;
    uint8 Flags;
};


enum operand_immediate_flags : uint8
{
    Immediate_IsSigned = 0x1,   // Is the value stored in the immediate signed?
    Immediate_IsJump = 0x2,     // Immediate.Value must be interpreted as an 8-bit signed value
};


struct instruction_operand
{
    operand_types Type = Operand_None;
    union
    {
        operand_memory Memory;
        register_id Register;
        operand_immediate Immediate;
    };
};


struct instruction_bits
{
    union
    {
        uint8 Bytes[6] = {};
        struct
        {
            uint8 Byte0;
            uint8 Byte1;
            uint8 Byte2;
            uint8 Byte3;
            uint8 Byte4;
            uint8 Byte5;
        };
    };
    uint8 *BytePtr = Bytes;
    uint8 ByteCount = 0;
};


struct instruction
{
    operation_types OpType = Op_unknown;
    instruction_operand Operands[2] = {};
    instruction_bits Bits = {};

    uint8 DirectionBit = 0;
    uint8 WidthBit = 0;
    uint8 ModBits = 0;
    uint8 RegBits = 0;
    uint8 RmBits = 0;
    uint8 SignBit = 0;

    uint8 ClockCount = 0;
    uint8 EAClockCount = 0;
};

#endif