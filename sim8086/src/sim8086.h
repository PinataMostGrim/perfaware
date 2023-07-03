/* TODO (Aaron):
    - Change all structs and enums to typedefs?
*/

#ifndef SIM8086_H
#define SIM8086_H

#include <assert.h>

#include "base.h"

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
    U8 RegisterIndex = 0;
    B32 IsWide = TRUE;
    U16 Mask = 0x0;
};


// Flags:
// OF | SF | ZF | AF | PF | CF
//     CF - Carry flag
//     PF - Parity flag
//     AF - Auxiliary Carry flag
//     ZF - Zero flag
//     SF - Sign flag
//     OF - Overflow flag
enum register_flags : U8
{
    RegisterFlag_CF = 0x1,
    RegisterFlag_PF = 0x2,
    RegisterFlag_AF = 0x4,
    RegisterFlag_ZF = 0x8,          // Did an arithmetic operation produce a value of 0?
    RegisterFlag_SF = 0x10,         // Did an arithmetic operation produce a negative value?
    RegisterFlag_OF = 0x20,
};


struct processor_8086
{
    union
    {
        U16 Registers[8] = {};   // Note (Aaron): Registers are: A, B, C, D, SP, BP, SI and DI
        struct
        {
            U16 RegisterAX;
            U16 RegisterBX;
            U16 RegisterCX;
            U16 RegisterDX;
            U16 RegisterSP;
            U16 RegisterBP;
            U16 RegisterSI;
            U16 RegisterDI;
        };
    };
    U8 Flags = 0;            // Note (Aaron): Register flags
    U32 IP = 0;              // Note (Aaron): Instruction pointer
    U32 PrevIP = 0;          // Note (Aaron): Previous instruction pointer

    // Note (Aaron): The 8086 had 16 memory segments of 64k bytes each. Memory segments could
    // be slid around like windows over the entire memory surface, however I'm not replicating
    // that here. I'm arbitrarily allocating 1 memory segment (64k bytes) for program memory
    // and the remaining 15 memory segments for the program to make use of.
    U32 MemorySize = Megabytes(1);
    U8 *Memory;
    U32 ProgramSize = 0;

    // Note (Aaron): Number of instructions decoded from the loaded program
    U32 InstructionCount = 0;

    // Note (Aaron): Number of clock cycles used by the loaded program
    U32 TotalClockCount = 0;
};


// TODO (Aaron): Define underlying type? U32?
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
    S16 Displacement;
    U16 DirectAddress;
    U8 Flags;
};


enum operand_memory_flags : U8
{
    Memory_HasDisplacement = 0x1,
    Memory_HasDirectAddress = 0x2,
    // TODO (Aaron): This may not be necessary if we should ALWAYS prepend the width
    Memory_PrependWidth = 0x4,
    Memory_IsWide = 0x8,
};


struct operand_immediate
{
    U16 Value;
    U8 Flags;
};


enum operand_immediate_flags : U8
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
        U8 Bytes[6] = {};
        struct
        {
            U8 Byte0;
            U8 Byte1;
            U8 Byte2;
            U8 Byte3;
            U8 Byte4;
            U8 Byte5;
        };
    };
    U8 *BytePtr = Bytes;
    U8 ByteCount = 0;
};


struct instruction
{
    U32 Address;

    operation_types OpType = Op_unknown;
    instruction_operand Operands[2] = {};
    instruction_bits Bits = {};

    U8 DirectionBit = 0;
    U8 WidthBit = 0;
    U8 ModBits = 0;
    U8 RegBits = 0;
    U8 RmBits = 0;
    U8 SignBit = 0;

    U8 ClockCount = 0;
    U8 EAClockCount = 0;

    char *InstructionMnemonic;
    char *BitsMnemonic;
};


global_function B32 DumpMemoryToFile(processor_8086 *processor, const char *filename);
global_function void ReadInstructionStream(processor_8086 *processor, instruction *instruction, U8 byteCount);
global_function void ParseRmBits(processor_8086 *processor, instruction *instruction, instruction_operand *operand);
global_function U8 CalculateEffectiveAddressClocks(instruction_operand *operand);
global_function instruction DecodeNextInstruction(processor_8086 *processor);
global_function void ExecuteInstruction(processor_8086 *processor, instruction *instruction, memory_arena *output);

global_function U16 GetMemory(processor_8086 *processor, U32 effectiveAddress, B32 wide);
global_function U16 GetRegisterValue(processor_8086 *processor, register_id targetRegister);
global_function U8 GetRegisterFlag(processor_8086 *processor, register_flags flag);

global_function B32 HasProcessorFinishedExecution(processor_8086 *processor);
global_function void ResetProcessorExecution(processor_8086 *processor);

#endif //SIM8086_H
