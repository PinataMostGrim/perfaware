/* TODO (Aaron):
    - Separate print functionality from ExecuteInstruction() method so PrintFlagDiffs() can be moved into cli_sim8086.cpp
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base.h"
#include "memory_arena.h"
#include "sim8086.h"
#include "sim8086_mnemonics.h"


// #pragma clang diagnostic ignored "-Wnull-dereference"

global_variable register_id RegMemTables[3][8]
{
    {
        Reg_al,
        Reg_cl,
        Reg_dl,
        Reg_bl,
        Reg_ah,
        Reg_ch,
        Reg_dh,
        Reg_bh,
    },
    {
        Reg_ax,
        Reg_cx,
        Reg_dx,
        Reg_bx,
        Reg_sp,
        Reg_bp,
        Reg_si,
        Reg_di,
    },
    {
        Reg_bx_si,
        Reg_bx_di,
        Reg_bp_si,
        Reg_bp_di,
        Reg_si,
        Reg_di,
        Reg_bp,
        Reg_bx,
    }
};


global_variable register_info RegisterLookup[21]
{
    // Note (Aaron): Order of registers must match the order defined in 'register_id' enum
    // in order for lookup to work
    { Reg_al, 0, FALSE, 0x00ff},
    { Reg_cl, 2, FALSE, 0x00ff},
    { Reg_dl, 3, FALSE, 0x00ff},
    { Reg_bl, 1, FALSE, 0x00ff},
    { Reg_ah, 0, FALSE, 0xff00},
    { Reg_ch, 2, FALSE, 0xff00},
    { Reg_dh, 3, FALSE, 0xff00},
    { Reg_bh, 1, FALSE, 0xff00},
    { Reg_ax, 0, true, 0xffff},
    { Reg_cx, 2, true, 0xffff},
    { Reg_dx, 3, true, 0xffff},
    { Reg_bx, 1, true, 0xffff},
    { Reg_sp, 4, true, 0xffff},
    { Reg_bp, 5, true, 0xffff},
    { Reg_si, 6, true, 0xffff},
    { Reg_di, 7, true, 0xffff},

    // Note (Aaron): As these entries are a combination of multiple registers,
    // the 'RegisterIndex' field must be handled as special cases.
    { Reg_bx_si, 0xff, true, 0xffff},
    { Reg_bx_di, 0xff, true, 0xffff},
    { Reg_bp_si, 0xff, true, 0xffff},
    { Reg_bp_di, 0xff, true, 0xffff},

    { Reg_unknown, 0, 0x0},
};

global_variable const char *MemoryDumpFilename = "memory.dat";


global_function B32 DumpMemoryToFile(processor_8086 *processor, const char *filename)
{
    FILE *file = {};
    file = fopen(filename, "wb");

    if(!file)
    {
        printf("ERROR: Unable to open '%s'\n", filename);
        return FALSE;
    }

    // loop over memory and write character to file stream one at a time
    for (U32 i = 0; i < processor->MemorySize; ++i)
    {
        U8 value = (U8)*(processor->Memory + i);
        fputc(value, file);
    }

    // error handling for file write
    if (ferror(file))
    {
        printf("ERROR: Encountered error while writing memory to '%s'", filename);
        return FALSE;
    }

    fclose(file);
    return true;
}


// Note (Aaron): Reads the next N bytes of the instruction stream into an instruction's bits.
// Advances both the instruction bits pointer and the processor's instruction pointer.
global_function void ReadInstructionStream(processor_8086 *processor, instruction *instruction, U8 byteCount)
{
    // Note (Aaron): Currently only support 8086 instructions that have a maximum of 6 bytes.
    // In practice, we never read this many bytes at once.
    assert_8086(byteCount < 6);

    // error out if we attempt to read outside of program memory
    if ((processor->IP + byteCount) > processor->ProgramSize)
    {
        printf("ERROR: Attempted to read outside of program memory (%i)\n", (processor->IP + byteCount));
        printf("\tprogram size: %i\n", processor->ProgramSize);
        printf("\tip: 0x%x (%i)\n",
               processor->IP,
               processor->IP);
        printf("\tbyteCount: %i\n", byteCount);

        Assert(FALSE && "Attempted to read outside of program memory");
        // TODO (Aaron): Need to support Win32 and not immediately exit
        exit(1);
    }

    // load instruction bytes out of memory
    U8 *readStartPtr = processor->Memory + processor->IP;
    MemoryCopy(instruction->Bits.BytePtr, readStartPtr, byteCount);

    // advance pointers and counters
    instruction->Bits.BytePtr += byteCount;
    processor->IP += byteCount;
    instruction->Bits.ByteCount++;
}


// TODO (Aaron): Pass in operand index rather than the operand itself

// Note (Aaron): Requires width, mod and rm to be decoded in the instruction first
global_function void ParseRmBits(processor_8086 *processor, instruction *instruction, instruction_operand *operand)
{
    // memory mode, no displacement
    if(instruction->ModBits == 0b0)
    {
        operand->Type = Operand_Memory;
        operand->Memory = {};
        operand->Memory.Flags |= Memory_PrependWidth;

        if (instruction->WidthBit)
        {
            operand->Memory.Flags |= Memory_IsWide;
        }

        // general case, memory mode no displacement
        if(instruction->RmBits != 0b110)
        {
            operand->Memory.Register = RegMemTables[2][instruction->RmBits];
        }
        // special case for direct address in memory mode with no displacement (R/M == 110)
        else
        {
            operand->Memory.Flags |= Memory_HasDirectAddress;

            // read direct address
            if (instruction->WidthBit == 0)
            {
                // Note (Aaron): Casey said that this special case is always a 16-bit displacement in Q&A #5 (at 27m30s)
                // but I think he meant the direct address value is always 16 bits. Putting an assert here to catch
                // it in case we ever do hit a width bit of 0 in this case.
                assert_8086(FALSE);
            }

            U8 *readStartPtr = instruction->Bits.BytePtr;
            ReadInstructionStream(processor, instruction, 2);
            operand->Memory.DirectAddress = (U16)(*(U16 *)readStartPtr);
        }
    }

    // memory mode, 8-bit and 16-bit displacement
    else if ((instruction->ModBits == 0b1) || (instruction->ModBits == 0b10))
    {
        operand->Type = Operand_Memory;
        operand->Memory = {};
        operand->Memory.Flags |= Memory_HasDisplacement;
        operand->Memory.Flags |= Memory_PrependWidth;

        if (instruction->WidthBit)
        {
            operand->Memory.Flags |= Memory_IsWide;
        }

        // read displacement value
        if (instruction->ModBits == 0b1)
        {
            U8 *readStartPtr = instruction->Bits.BytePtr;
            ReadInstructionStream(processor, instruction, 1);
            operand->Memory.Displacement = (S16)(*(S8 *)readStartPtr);
        }
        else if (instruction->ModBits == 0b10)
        {
            U8 *readStartPtr = instruction->Bits.BytePtr;
            ReadInstructionStream(processor, instruction, 2);
            operand->Memory.Displacement = (S16)(*(S16 *)readStartPtr);
        }

        operand->Memory.Register = RegMemTables[2][instruction->RmBits];
    }

    // register mode, no displacement
    else if (instruction->ModBits == 0b11)
    {
        operand->Type = Operand_Register;
        operand->Register = RegMemTables[instruction->WidthBit][instruction->RmBits];
    }
}


global_function U8 CalculateEffectiveAddressClocks(instruction_operand *operand)
{
    // TODO (Aaron): We could just return 0 clocks instead in the event this is false
    // if (operand->Type != Operand_Memory)
    // {
    //     return 0;
    // }
    assert_8086(operand->Type == Operand_Memory);

    // TODO (Aaron): Is "Displacement Only" in table 2-20 the same thing as direct address?
    if (operand->Memory.Flags & Memory_HasDirectAddress)
    {
        return 6;
    }

    register_id operandRegister = operand->Memory.Register;
    B32 baseOrIndex = operandRegister ==  Reg_bx
                    || operandRegister ==  Reg_bp
                    || operandRegister ==  Reg_si
                    || operandRegister ==  Reg_di;
    B32 hasDisplacement = (operand->Memory.Flags & Memory_HasDisplacement) && operand->Memory.Displacement > 0;

    if (baseOrIndex)
    {
        // base or index with no displacement is 5 clocks
        // base or index with displacement is 9 clocks
        return hasDisplacement ? 9 : 5;
    }

    // base + index
    // base + index + displacement
    switch (operandRegister)
    {
        case Reg_bp_di:
        case Reg_bx_si:
            return hasDisplacement ? 7 : 11;
        case Reg_bp_si:
        case Reg_bx_di:
            return hasDisplacement ? 8 : 12;
        default:
            break;
    }

    assert_8086(FALSE && "Unreachable case");
    return 0;
}


global_function instruction DecodeNextInstruction(processor_8086 *processor)
{
    processor->PrevIP = processor->IP;
    instruction instruction = {};
    instruction.Address = processor->IP;

    // read initial instruction byte for parsing
    processor->InstructionCount++;
    ReadInstructionStream(processor, &instruction, 1);

    // mov instruction - register/memory to/from register (0b100010)
    if ((instruction.Bits.Byte0 >> 2) == 0b100010)
    {
        instruction.OpType = Op_mov;

        instruction_operand operandReg = {};
        instruction_operand operandRm = {};

        // parse initial instruction byte
        instruction.DirectionBit = (instruction.Bits.Byte0 >> 1) & 0b1;
        instruction.WidthBit = instruction.Bits.Byte0 & 0b1;

        // read second instruction byte and parse it
        ReadInstructionStream(processor, &instruction, 1);

        instruction.ModBits = (instruction.Bits.Byte1 >> 6);
        instruction.RegBits = (instruction.Bits.Byte1 >> 3) & 0b111;
        instruction.RmBits = instruction.Bits.Byte1 & 0b111;

        // decode reg
        operandReg.Type = Operand_Register;
        operandReg.Register = RegMemTables[instruction.WidthBit][instruction.RegBits];

        // parse r/m
        ParseRmBits(processor, &instruction, &operandRm);

        // set dest and source strings
        // destination is in RM field, source is in REG field
        if (instruction.DirectionBit == 0)
        {
            instruction.Operands[0] = operandRm;
            instruction.Operands[1] = operandReg;
        }
        // destination is in REG field, source is in RM field
        else
        {
            instruction.Operands[0] = operandReg;
            instruction.Operands[1] = operandRm;
        }

        // estimate clock cycles
        // register to register
        if (instruction.Operands[0].Type == Operand_Register
            && instruction.Operands[1].Type == Operand_Register)
        {
            instruction.ClockCount = 2;
        }
        // register to memory
        else if (instruction.Operands[0].Type == Operand_Memory
                 && instruction.Operands[1].Type == Operand_Register)
        {
            instruction.ClockCount = 9;
            instruction.EAClockCount = CalculateEffectiveAddressClocks(&instruction.Operands[0]);
        }
        // memory to register
        else if (instruction.Operands[0].Type == Operand_Register
            && instruction.Operands[1].Type == Operand_Memory)
        {
            instruction.ClockCount = 8;
            instruction.EAClockCount = CalculateEffectiveAddressClocks(&instruction.Operands[1]);
        }
        else
        {
            assert_8086(FALSE && "Unreachable case");
        }
    }

    // mov instruction - immediate to register/memory (0b1100011)
    else if ((instruction.Bits.Byte0 >> 1) == 0b1100011)
    {
        instruction.OpType = Op_mov;
        instruction_operand operandSource = {};
        instruction_operand operandDest = {};
        operandSource.Type = Operand_Immediate;

        instruction.WidthBit = instruction.Bits.Byte0 & 0b1;

        // read second instruction byte and parse it
        ReadInstructionStream(processor, &instruction, 1);

        instruction.ModBits = (instruction.Bits.Byte1 >> 6);
        instruction.RmBits = (instruction.Bits.Byte1) & 0b111;

        // Note (Aaron): No reg in this instruction

        // parse r/m
        ParseRmBits(processor, &instruction, &operandDest);

        // read data. guaranteed to be at least 8-bits.
        if (instruction.WidthBit == 0b0)
        {
            U8 *readStartPtr = instruction.Bits.BytePtr;
            ReadInstructionStream(processor, &instruction, 1);
            operandSource.Immediate.Value = (U16)(*(U8 *)readStartPtr);
        }
        else if (instruction.WidthBit == 0b1)
        {
            U8 *readStartPtr = instruction.Bits.BytePtr;
            ReadInstructionStream(processor, &instruction, 2);
            operandSource.Immediate.Value = (U16)(*(U16 *)readStartPtr);
        }
        else
        {
            assert_8086(FALSE && "Unhandled case");
        }

        instruction.Operands[0] = operandDest;
        instruction.Operands[1] = operandSource;

        // estimate clock cycles
        // immediate to register
        if (instruction.Operands[0].Type == Operand_Register
            && instruction.Operands[1].Type == Operand_Immediate)
        {
            instruction.ClockCount = 4;
        }
        // immediate to memory
        else if (instruction.Operands[0].Type == Operand_Memory
            && instruction.Operands[1].Type == Operand_Immediate)
        {
            instruction.ClockCount = 10;
            instruction.EAClockCount = CalculateEffectiveAddressClocks(&instruction.Operands[0]);
        }
        else
        {
            assert_8086(FALSE && "Unreachable case");
        }
    }

    // mov instruction - immediate to register (0b1011)
    else if ((instruction.Bits.Byte0 >> 4) == 0b1011)
    {
        instruction.OpType = Op_mov;
        instruction_operand operandSource = {};
        instruction_operand operandDest = {};
        operandSource.Type = Operand_Immediate;
        operandDest.Type = Operand_Register;

        // parse width and reg
        instruction.WidthBit = (instruction.Bits.Byte0 >> 3) & (0b1);
        instruction.RegBits = instruction.Bits.Byte0 & 0b111;

        if (instruction.WidthBit == 0b0)
        {
            // read 8-bit data
            ReadInstructionStream(processor, &instruction, 1);
            operandSource.Immediate.Value = *(U8 *)(&instruction.Bits.Byte1);
        }
        else if (instruction.WidthBit == 0b1)
        {
            // read 16-bit data
            ReadInstructionStream(processor, &instruction, 2);
            operandSource.Immediate.Value = *(U16 *)(&instruction.Bits.Byte1);
        }
        else
        {
            assert_8086(FALSE && "Unhandled case");
        }

        operandDest.Register = RegMemTables[instruction.WidthBit][instruction.RegBits];
        instruction.Operands[0] = operandDest;
        instruction.Operands[1] = operandSource;

        // estimate clock cycles
        // immediate to register
        if (instruction.Operands[0].Type == Operand_Register
            && instruction.Operands[1].Type == Operand_Immediate)
        {
            instruction.ClockCount = 4;
        }
        else
        {
            assert_8086(FALSE && "Unreachable case");
        }
    }

    // mov - memory to accumulator and accumulator to memory
    else if (((instruction.Bits.Byte0 >> 1) == 0b1010000) || ((instruction.Bits.Byte0 >> 1) == 0b1010001))
    {
        instruction.OpType = Op_mov;
        instruction_operand operandAccumulator = {};
        instruction_operand operandMemory = {};
        operandAccumulator.Type = Operand_Register;
        operandAccumulator.Register = Reg_ax;
        operandMemory.Type = Operand_Memory;
        operandMemory.Memory.Flags |= Memory_HasDirectAddress;

        instruction.WidthBit = instruction.Bits.Byte0 & 0b1;
        if (instruction.WidthBit == 0)
        {
            ReadInstructionStream(processor, &instruction, 1);
            operandMemory.Memory.DirectAddress = (U16)(*(&instruction.Bits.Byte1));
        }
        else if (instruction.WidthBit == 1)
        {
            ReadInstructionStream(processor, &instruction, 2);
            operandMemory.Memory.DirectAddress = (U16)(*(U16 *)(&instruction.Bits.Byte1));
        }
        else
        {
            assert_8086(FALSE && "Unhandled case");
        }

        if ((instruction.Bits.Byte0 >> 1) == 0b1010000)
        {
            instruction.Operands[0] = operandAccumulator;
            instruction.Operands[1] = operandMemory;
        }
        else if ((instruction.Bits.Byte0 >> 1) == 0b1010001)
        {
            instruction.Operands[0] = operandMemory;
            instruction.Operands[1] = operandAccumulator;
        }
        else
        {
            assert_8086(FALSE && "Unhandled case");
        }

        // estimate clock cycles
        // Note (Aaron): Both memory to accumulator and accumulator to memory share
        // the same clock count
        instruction.ClockCount = 10;
    }

    // add / sub / cmp - reg/memory with register to either
    else if (((instruction.Bits.Byte0 >> 2) == 0b000000)
        || ((instruction.Bits.Byte0 >> 2) == 0b001010)
        || ((instruction.Bits.Byte0 >> 2) == 0b001110))
    {
        instruction_operand operandReg = {};
        operandReg.Type = Operand_Register;
        instruction_operand operandRm = {};

        // decode direction and width
        instruction.DirectionBit = (instruction.Bits.Byte0 >> 1) & 0b1;
        instruction.WidthBit = instruction.Bits.Byte0 & 0b1;

        // decode mod, reg and r/m
        ReadInstructionStream(processor, &instruction, 1);
        instruction.ModBits = (instruction.Bits.Byte1 >> 6) & 0b11;
        instruction.RegBits = (instruction.Bits.Byte1 >> 3) & 0b111;
        instruction.RmBits = instruction.Bits.Byte1 & 0b111;

        // decode instruction type
        // add = 0b000
        if (((instruction.Bits.Byte0 >> 3) & 0b111) == 0b000)
        {
            instruction.OpType = Op_add;
        }
        // sub = 0b101
        else if (((instruction.Bits.Byte0 >> 3) & 0b111) == 0b101)
        {
            instruction.OpType = Op_sub;
        }
        // cmp = 0b111
        else if (((instruction.Bits.Byte0 >> 3) & 0b111) == 0b111)
        {
            instruction.OpType = Op_cmp;
        }
        else
        {
            assert_8086(FALSE && "Unhandled case");
        }

        // decode reg
        operandReg.Register = RegMemTables[instruction.WidthBit][instruction.RegBits];

        // parse r/m
        ParseRmBits(processor, &instruction, &operandRm);

        // set dest and source strings
        // destination is in RM field, source is in REG field
        if (instruction.DirectionBit == 0)
        {
            instruction.Operands[0] = operandRm;
            instruction.Operands[1] = operandReg;
        }
        // destination is in REG field, source is in RM field
        else
        {
            instruction.Operands[0] = operandReg;
            instruction.Operands[1] = operandRm;
        }

        // estimate clock cycles
        // register to register
        if (instruction.Operands[0].Type == Operand_Register
            && instruction.Operands[1].Type == Operand_Register)
        {
            // Note (Aaron): add, sub, and cmp share the same clock count for this operation
            instruction.ClockCount = 3;
        }
        // register to memory
        else if (instruction.Operands[0].Type == Operand_Memory
            && instruction.Operands[1].Type == Operand_Register)
        {
            instruction.ClockCount = 16;
            switch (instruction.OpType)
            {
                case Op_add:
                case Op_sub:
                    instruction.ClockCount = 16;
                    break;
                case Op_cmp:
                    instruction.ClockCount = 9;
                    break;
                default:
                    assert_8086(FALSE && "Unhandled case");
            }

            instruction.EAClockCount = CalculateEffectiveAddressClocks(&instruction.Operands[0]);
        }
        // memory to register
        else if (instruction.Operands[0].Type == Operand_Register
            && instruction.Operands[1].Type == Operand_Memory)
        {
            // Note (Aaron): add, sub, and cmp share the same clock count for this operation
            instruction.ClockCount = 9;
            instruction.EAClockCount = CalculateEffectiveAddressClocks(&instruction.Operands[1]);
        }
        else
        {
            assert_8086(FALSE && "Unreachable case");
        }
    }

    // add / sub / cmp - immediate to register/memory
    else if ((instruction.Bits.Byte0 >> 2) == 0b100000)
    {
        instruction_operand operandSource = {};
        instruction_operand operandDest = {};
        operandSource.Type = Operand_Immediate;

        instruction.SignBit = (instruction.Bits.Byte0 >> 1) & 0b1;
        instruction.WidthBit = instruction.Bits.Byte0 & 0b1;

        // decode mod and r/m
        ReadInstructionStream(processor, &instruction, 1);

        instruction.ModBits = (instruction.Bits.Byte1 >> 6) & 0b11;
        instruction.RmBits = instruction.Bits.Byte1 & 0b111;

        // decode instruction type
        // add = 0b000
        if (((instruction.Bits.Byte1 >> 3) & 0b111) == 0b000)
        {
            instruction.OpType = Op_add;
        }
        // sub = 0b101
        else if (((instruction.Bits.Byte1 >> 3) & 0b111) == 0b101)
        {
            instruction.OpType = Op_sub;
        }
        // cmp = 0b111
        else if (((instruction.Bits.Byte1 >> 3) & 0b111) == 0b111)
        {
            instruction.OpType = Op_cmp;
        }
        else
        {
            assert_8086(FALSE && "Unhandled case");
        }

        // parse r/m string
        ParseRmBits(processor, &instruction, &operandDest);

        // read data.
        if (instruction.SignBit == 0b0 && instruction.WidthBit == 0)
        {
            // read 8-bit unsigned
            U8 *readStartPtr = instruction.Bits.BytePtr;
            ReadInstructionStream(processor, &instruction, 1);
            operandSource.Immediate.Value = (U16)(*readStartPtr);
        }
        else if (instruction.SignBit == 0b0 && instruction.WidthBit == 1)
        {
            // read 16-bit unsigned
            U8 *readStartPtr = instruction.Bits.BytePtr;
            ReadInstructionStream(processor, &instruction, 2);
            operandSource.Immediate.Value = (U16)(*(U16 *)readStartPtr);
        }
        else if (instruction.SignBit == 0b1 && instruction.WidthBit == 0)
        {
            // read 8-bit signed
            U8 *readStartPtr = instruction.Bits.BytePtr;
            ReadInstructionStream(processor, &instruction, 1);
            operandSource.Immediate.Value = (U16)((S8)(*readStartPtr));
            operandSource.Immediate.Flags |= Immediate_IsSigned;
        }
        else if (instruction.SignBit == 0b1 && instruction.WidthBit == 1)
        {
            // read 8-bits and sign-extend to 16-bits
            U8 *readStartPtr = instruction.Bits.BytePtr;
            ReadInstructionStream(processor, &instruction, 1);
            operandSource.Immediate.Value = (U16)((S8)(*readStartPtr));
            operandSource.Immediate.Flags |= Immediate_IsSigned;
        }

        instruction.Operands[0] = operandDest;
        instruction.Operands[1] = operandSource;

        // estimate clock cycles
        // immediate to register
        if (instruction.Operands[0].Type == Operand_Register)
        {
            // Note (Aaron): add, sub, and cmp share the same clock count
            instruction.ClockCount = 4;
        }
        // immediate to memory
        else if (instruction.Operands[0].Type == Operand_Memory)
        {
            switch (instruction.OpType)
            {
                case Op_add:
                case Op_sub:
                    instruction.ClockCount = 17;
                    break;
                case Op_cmp:
                    instruction.ClockCount = 10;
                    break;
                default:
                    assert_8086(FALSE && "Unhandled case");
            }

            instruction.EAClockCount = CalculateEffectiveAddressClocks(&instruction.Operands[0]);
        }
        else
        {
            assert_8086(FALSE && "Unreachable case");
        }
    }

    // add / sub / cmp - immediate to/from/with accumulator
    else if (((instruction.Bits.Byte0 >> 1) == 0b0000010)
        || ((instruction.Bits.Byte0 >> 1) == 0b0010110)
        || ((instruction.Bits.Byte0 >> 1) == 0b0011110))
    {
        instruction.WidthBit = instruction.Bits.Byte0 & 0b1;

        instruction_operand operandSource = {};
        instruction_operand operandDest = {};
        operandSource.Type = Operand_Immediate;
        operandDest.Type = Operand_Register;

        // decode instruction type
        // add = 0b000
        if (((instruction.Bits.Byte0 >> 3) & 0b111) == 0b000)
        {
            instruction.OpType = Op_add;
        }
        // sub = 0b101
        else if (((instruction.Bits.Byte0 >> 3) & 0b111) == 0b101)
        {
            instruction.OpType = Op_sub;
        }
        // cmp = 0b111
        else if (((instruction.Bits.Byte0 >> 3) & 0b111) == 0b111)
        {
            instruction.OpType = Op_cmp;
        }
        else
        {
            assert_8086(FALSE && "Unhandled case");
        }

        // read data
        if (instruction.WidthBit == 0b0)
        {
            ReadInstructionStream(processor, &instruction, 1);
            operandSource.Immediate.Value = (U16)(*(U8 *)(&instruction.Bits.Byte1));
        }
        else if (instruction.WidthBit == 0b1)
        {
            ReadInstructionStream(processor, &instruction, 2);
            operandSource.Immediate.Value = (U16)(*(U16 *)(&instruction.Bits.Byte1));
        }

        operandDest.Register = (instruction.WidthBit == 0b0) ? Reg_al : Reg_ax;

        instruction.Operands[0] = operandDest;
        instruction.Operands[1] = operandSource;

        // estimate clock cycles
        // Note (Aaron): add, sub, and cmp share the same clock count
        instruction.ClockCount = 4;
    }

    // control transfer instructions
    else if ((instruction.Bits.Byte0 == 0b01110101)     // jnz / jne
             || (instruction.Bits.Byte0 == 0b01110100)  // je
             || (instruction.Bits.Byte0 == 0b01111100)  // jl
             || (instruction.Bits.Byte0 == 0b01111110)  // jle
             || (instruction.Bits.Byte0 == 0b01110010)  // jb
             || (instruction.Bits.Byte0 == 0b01110110)  // jbe
             || (instruction.Bits.Byte0 == 0b01111010)  // jp
             || (instruction.Bits.Byte0 == 0b01110000)  // jo
             || (instruction.Bits.Byte0 == 0b01111000)  // js
             || (instruction.Bits.Byte0 == 0b01111101)  // jnl
             || (instruction.Bits.Byte0 == 0b01111111)  // jg
             || (instruction.Bits.Byte0 == 0b01110011)  // jnb
             || (instruction.Bits.Byte0 == 0b01110111)  // ja
             || (instruction.Bits.Byte0 == 0b01111011)  // jnp
             || (instruction.Bits.Byte0 == 0b01110001)  // jno
             || (instruction.Bits.Byte0 == 0b01111001)  // jns
             || (instruction.Bits.Byte0 == 0b11100010)  // loop
             || (instruction.Bits.Byte0 == 0b11100001)  // loopz
             || (instruction.Bits.Byte0 == 0b11100000)  // loopnz
             || (instruction.Bits.Byte0 == 0b11100011)) // jcxz
    {
        // TODO (Aaron): Add clock count estimation for these jumps

        instruction.OpType = Op_unknown;
        char instructionStr[32] = "";

        // read 8-bit signed offset for jumps
        ReadInstructionStream(processor, &instruction, 1);
        S8 offset = *(S8 *)(&instruction.Bits.Byte1);

        instruction_operand operand0 = {};
        operand0.Type = Operand_Immediate;
        operand0.Immediate.Flags |= Immediate_IsJump;
        // Note (Aaron): A signed 8-bit value will need to be extracted from
        // the unsigned 16-bit ImmediateValue when instructions are executed.
        operand0.Immediate.Value = offset;
        operand0.Immediate.Flags |= Immediate_IsSigned;
        instruction.Operands[0] = operand0;

        instruction_operand operand1 = {};
        operand1.Type = Operand_None;
        instruction.Operands[1] = operand1;

        switch(instruction.Bits.Byte0)
        {
            // je / jz
            case 0b01110100:
                instruction.OpType = Op_je;
                break;

            // jl / jnge
            case 0b01111100:
                instruction.OpType = Op_jl;
                break;

            // jle / jng
            case 0b01111110:
                instruction.OpType = Op_jle;
                break;

            // jb / jnae
            case 0b01110010:
                instruction.OpType = Op_jb;
                break;

            // jbe / jna
            case 0b01110110:
                instruction.OpType = Op_jbe;
                break;

            // jp / jpe
            case 0b01111010:
                instruction.OpType = Op_jp;
                break;

            // jo
            case 0b01110000:
                instruction.OpType = Op_jo;
                break;

            // js
            case 0b01111000:
                instruction.OpType = Op_js;
                break;

            // jne / jnz
            case 0b01110101:
                instruction.OpType = Op_jne;
                break;

            // jnl / jge
            case 0b01111101:
                instruction.OpType = Op_jnl;
                break;

            // jnle / jg
            case 0b01111111:
                instruction.OpType = Op_jg;
                break;

            // jnb / jae
            case 0b01110011:
                instruction.OpType = Op_jnb;
                break;

            // jnbe / ja
            case 0b01110111:
                instruction.OpType = Op_ja;
                break;

            // jnp / jpo
            case 0b01111011:
                instruction.OpType = Op_jnp;
                break;

            // jno
            case 0b01110001:
                instruction.OpType = Op_jno;
                break;

            // jns
            case 0b01111001:
                instruction.OpType = Op_jns;
                break;

            // loop
            case 0b11100010:
                instruction.OpType = Op_loop;
                break;

            // loopz
            case 0b11100001:
                instruction.OpType = Op_loopz;
                break;

            // loopnz
            case 0b11100000:
                instruction.OpType = Op_loopnz;
                break;

            // jcxz
            case 0b11100011:
                instruction.OpType = Op_jcxz;
                break;
            default:
                assert_8086(FALSE && "Unhandled instruction");
        }
    }

    // return instructions
    else if ((instruction.Bits.Byte0 == 0b11000011) // ret within segment
        || (instruction.Bits.Byte0 == 0b11000010)   // ret with segment adding immediate to SP
        || (instruction.Bits.Byte0 == 0b11001011)   // ret with inter-segment
        || (instruction.Bits.Byte0 == 0b11001010))  // ret with inter-segment adding immediate to SP
    {
        instruction.OpType = Op_ret;
        instruction_operand operand_none = {};

        if (instruction.Bits.Byte0 == 0b11000010
            || instruction.Bits.Byte0 == 0b11001010)
        {
            // Note (Aaron): Some of these instructions include additional bytes. Because we are stopping the sim
            // on ret instructions, they do not need to be implemented at the moment.
            ReadInstructionStream(processor, &instruction, 2);
        }

        operand_none.Type = Operand_None;
        instruction.Operands[0] = operand_none;
        instruction.Operands[1] = operand_none;
    }

    // unsupported instruction
    else
    {
        instruction.OpType = Op_unknown;
        instruction_operand unknown = {};
        instruction.Operands[0] = unknown;
        instruction.Operands[1] = unknown;
    }

    processor->TotalClockCount += (instruction.ClockCount + instruction.EAClockCount);

    return instruction;
}


global_function U16 GetRegisterValue(processor_8086 *processor, register_id targetRegister)
{
    U16 result = 0;
    register_info info = RegisterLookup[targetRegister];

    switch (info.Register)
    {
        case (Reg_bx_si):
        {
            U16 bxValue = GetRegisterValue(processor, Reg_bx);
            U16 siValue = GetRegisterValue(processor, Reg_si);
            result = bxValue + siValue;
            break;
        }
        case (Reg_bx_di):
        {
            U16 bxValue = GetRegisterValue(processor, Reg_bx);
            U16 diValue = GetRegisterValue(processor, Reg_di);
            result = bxValue + diValue;
            break;
        }
        case (Reg_bp_si):
        {
            U16 bpValue = GetRegisterValue(processor, Reg_bp);
            U16 siValue = GetRegisterValue(processor, Reg_si);
            result = bpValue + siValue;
            break;
        }
        case (Reg_bp_di):
        {
            U16 bpValue = GetRegisterValue(processor, Reg_bp);
            U16 diValue = GetRegisterValue(processor, Reg_di);
            result = bpValue + diValue;
            break;
        }
        default:
        {
            result = (processor->Registers[info.RegisterIndex] & info.Mask);
            break;
        }
    }

    return result;
}


global_function void SetRegisterValue(processor_8086 *processor, register_id targetRegister, U16 value)
{
    register_info info = RegisterLookup[targetRegister];
    // TODO (Aaron): I don't think this preserves values in a lower or higher segment of the register
    // if you are only wanting to write to one segment.
    //  - Can retrieve the register value first, invert the mask and then combine the values to fix this
    processor->Registers[info.RegisterIndex] = (value & info.Mask);
}


global_function U8 GetRegisterFlag(processor_8086 *processor, register_flags flag)
{
    return (processor->Flags & flag);
}


global_function void SetRegisterFlag(processor_8086 *processor, register_flags flag, B32 set)
{
    if (set)
    {
        processor->Flags |= flag;
        return;
    }

    processor->Flags &= ~flag;
}


global_function void UpdateSignedRegisterFlag(processor_8086 *processor, register_id targetRegister, U16 value)
{
    register_info info = RegisterLookup[targetRegister];
    if (info.IsWide)
    {
        // use 16 bit mask
        SetRegisterFlag(processor, RegisterFlag_SF, (value >> 15) == 1);
    }
    else
    {
        // use 8 bit mask
        SetRegisterFlag(processor, RegisterFlag_SF, (value >> 7) == 1);
    }
}


global_function U32 CalculateEffectiveAddress(processor_8086 *processor, instruction_operand operand)
{
    assert_8086(operand.Type == Operand_Memory);

    U32 effectiveAddress = 0;
    // direct address assignment
    if (operand.Memory.Flags & Memory_HasDirectAddress)
    {
        effectiveAddress = operand.Memory.DirectAddress;
    }
    // effective address calculation
    else
    {
        effectiveAddress = GetRegisterValue(processor, operand.Memory.Register);
        if (operand.Memory.Flags & Memory_HasDisplacement)
        {
            effectiveAddress += operand.Memory.Displacement;
        }
    }

    return effectiveAddress;
}


global_function U16 GetMemory(processor_8086 *processor, U32 effectiveAddress, B32 wide)
{
    if (effectiveAddress >= processor->MemorySize)
    {
        printf("[ERROR] Attempted to load out-of-bounds memory: 0x%x (out of 0x%x)", effectiveAddress, processor->MemorySize);
        Assert(FALSE && "Attempted to load out-of-bounds memory");
        // TODO (Aaron): Need to support Win32 and not immediately exit
        exit(1);
    }

    if (wide)
    {
        U16 *memoryRead = (U16 *)(processor->Memory + effectiveAddress);
        U16 result = *memoryRead;

        return result;
    }

    U8 *memoryRead = (processor->Memory + effectiveAddress);
    U16 result = (U16)*memoryRead;

    return result;
}


global_function void SetMemory(processor_8086 *processor, U32 effectiveAddress, U16 value, B32 wide)
{
    if (effectiveAddress >= processor->MemorySize)
    {
        printf("[ERROR] Attempted to set out-of-bounds memory: 0x%x (out of 0x%x)", effectiveAddress, processor->MemorySize);
        Assert(FALSE && "Attempted to set out-of-bounds memory");
        // TODO (Aaron): Need to support Win32 and not immediately exit
        exit(1);
    }

    // this should be valid as well but I'm not sure about the syntax
    // processor->Memory[effectiveAddress] = value;

    if (wide)
    {
        U16 *memoryWrite = (U16 *)(processor->Memory + effectiveAddress);
        *memoryWrite = value;
        return;
    }

    U8 *memoryWrite = processor->Memory + effectiveAddress;
    *memoryWrite = (U8)value;
}


global_function U16 GetOperandValue(processor_8086 *processor, instruction_operand operand)
{
    U16 result = 0;
    switch (operand.Type)
    {
        case Operand_Register:
        {
            result = GetRegisterValue(processor, operand.Register);
            break;
        }
        case Operand_Immediate:
        {
            result = operand.Immediate.Value;
            break;
        }
        case Operand_Memory:
        {
            U32 effectiveAddress = CalculateEffectiveAddress(processor, operand);
            B32 wide = (operand.Memory.Flags & Memory_IsWide);
            result = GetMemory(processor, effectiveAddress, wide);

            break;
        }
        default:
        {
            assert_8086(FALSE && "Invalid instruction");
            break;
        }
    }

    return result;
}


global_function void SetOperandValue(processor_8086 *processor, instruction_operand *operand, U16 value)
{
    // Only these two operand types are assignable
    assert_8086((operand->Type == Operand_Register) || operand->Type == Operand_Memory);

    if (operand->Type == Operand_Register)
    {
        SetRegisterValue(processor, operand->Register, value);
        return;
    }

    if (operand->Type == Operand_Memory)
    {
        U32 effectiveAddress = 0;
        B32 wide = (operand->Memory.Flags & Memory_IsWide);

        // direct address assignment
        if (operand->Memory.Flags & Memory_HasDirectAddress)
        {
            effectiveAddress = operand->Memory.DirectAddress;
            SetMemory(processor, effectiveAddress, value, wide);
            return;
        }

        // effective address calculation
        effectiveAddress = GetRegisterValue(processor, operand->Memory.Register);
        if (operand->Memory.Flags & Memory_HasDisplacement)
        {
            effectiveAddress += operand->Memory.Displacement;
        }

        SetMemory(processor, effectiveAddress, value, wide);
        return;
    }

    assert_8086(FALSE && "Unreachable case");
}


// TODO (Aaron): Potentially move into memory_arena.c?
// TODO (Aaron): Test this
global_function char *ArenaCircularPushString(memory_arena *arena, char *str, B8 concat)
{
    size_t length = strlen(str);
    Assert((arena->Size > arena->Used) && "Memory arena cannot use more space than it has allocated");
    size_t unused = arena->Size - arena->Used;
    if (length > unused)
    {
        ArenaClear(arena);
    }

    if (concat)
    {
        return ArenaPushStringConcat(arena, str);
    }
    else
    {
        return ArenaPushString(arena, str);
    }
}


global_function void PrintFlagDiffs(U8 oldFlags, U8 newFlags, memory_arena *outputArena)
{
    if (oldFlags == newFlags)
    {
        return;
    }

    // printf(" flags:");
    ArenaCircularPushString(outputArena, (char *)" flags:", TRUE);

    // Flags that are changing to 0
    // if ((oldFlags & RegisterFlag_CF) && !(newFlags & RegisterFlag_CF)) { printf("C"); }
    // if ((oldFlags & RegisterFlag_PF) && !(newFlags & RegisterFlag_PF)) { printf("P"); }
    // if ((oldFlags & RegisterFlag_AF) && !(newFlags & RegisterFlag_AF)) { printf("A"); }
    // if ((oldFlags & RegisterFlag_ZF) && !(newFlags & RegisterFlag_ZF)) { printf("Z"); }
    // if ((oldFlags & RegisterFlag_SF) && !(newFlags & RegisterFlag_SF)) { printf("S"); }
    // if ((oldFlags & RegisterFlag_OF) && !(newFlags & RegisterFlag_OF)) { printf("O"); }

    if ((oldFlags & RegisterFlag_CF) && !(newFlags & RegisterFlag_CF)) { ArenaCircularPushString(outputArena, (char *)"C", TRUE); }
    if ((oldFlags & RegisterFlag_PF) && !(newFlags & RegisterFlag_PF)) { ArenaCircularPushString(outputArena, (char *)"P", TRUE); }
    if ((oldFlags & RegisterFlag_AF) && !(newFlags & RegisterFlag_AF)) { ArenaCircularPushString(outputArena, (char *)"A", TRUE); }
    if ((oldFlags & RegisterFlag_ZF) && !(newFlags & RegisterFlag_ZF)) { ArenaCircularPushString(outputArena, (char *)"Z", TRUE); }
    if ((oldFlags & RegisterFlag_SF) && !(newFlags & RegisterFlag_SF)) { ArenaCircularPushString(outputArena, (char *)"S", TRUE); }
    if ((oldFlags & RegisterFlag_OF) && !(newFlags & RegisterFlag_OF)) { ArenaCircularPushString(outputArena, (char *)"O", TRUE); }


    // printf("->");
    ArenaCircularPushString(outputArena, (char *)"->", TRUE);

    // Flags that are changing to 1
    // if (!(oldFlags & RegisterFlag_CF) && (newFlags & RegisterFlag_CF)) { printf("C"); }
    // if (!(oldFlags & RegisterFlag_PF) && (newFlags & RegisterFlag_PF)) { printf("P"); }
    // if (!(oldFlags & RegisterFlag_AF) && (newFlags & RegisterFlag_AF)) { printf("A"); }
    // if (!(oldFlags & RegisterFlag_ZF) && (newFlags & RegisterFlag_ZF)) { printf("Z"); }
    // if (!(oldFlags & RegisterFlag_SF) && (newFlags & RegisterFlag_SF)) { printf("S"); }
    // if (!(oldFlags & RegisterFlag_OF) && (newFlags & RegisterFlag_OF)) { printf("O"); }

    if (!(oldFlags & RegisterFlag_CF) && (newFlags & RegisterFlag_CF)) { ArenaCircularPushString(outputArena, (char *)"C", TRUE); }
    if (!(oldFlags & RegisterFlag_PF) && (newFlags & RegisterFlag_PF)) { ArenaCircularPushString(outputArena, (char *)"P", TRUE); }
    if (!(oldFlags & RegisterFlag_AF) && (newFlags & RegisterFlag_AF)) { ArenaCircularPushString(outputArena, (char *)"A", TRUE); }
    if (!(oldFlags & RegisterFlag_ZF) && (newFlags & RegisterFlag_ZF)) { ArenaCircularPushString(outputArena, (char *)"Z", TRUE); }
    if (!(oldFlags & RegisterFlag_SF) && (newFlags & RegisterFlag_SF)) { ArenaCircularPushString(outputArena, (char *)"S", TRUE); }
    if (!(oldFlags & RegisterFlag_OF) && (newFlags & RegisterFlag_OF)) { ArenaCircularPushString(outputArena, (char *)"O", TRUE); }
}


global_function void ExecuteInstruction(processor_8086 *processor, instruction *instruction, memory_arena *outputArena)
{
    // TODO (Aaron): Decide how to pick a better value here
    const U8 BUFFER_SIZE = 64;
    char buffer[BUFFER_SIZE];
    U8 oldFlags = processor->Flags;

    // TODO (Aaron): A lot of redundant code here
    //  - Re-write switch statement with if-statements?
    switch (instruction->OpType)
    {
        case Op_mov:
        {
            instruction_operand operand0 = instruction->Operands[0];
            instruction_operand operand1 = instruction->Operands[1];

            U16 oldValue = GetOperandValue(processor, operand0);
            U16 sourceValue = GetOperandValue(processor, operand1);

            SetOperandValue(processor, &operand0, sourceValue);

            // Note (Aaron): mov does not modify the zero flag or the signed flag

            if (operand0.Type == Operand_Register && oldValue != sourceValue)
            {
                // printf(" %s:0x%x->0x%x",
                //        GetRegisterMnemonic(operand0.Register),
                //        oldValue,
                //        sourceValue);

                snprintf(buffer, BUFFER_SIZE,
                         " %s:0x%x->0x%x",
                         GetRegisterMnemonic(operand0.Register),
                         oldValue,
                         sourceValue);

                ArenaCircularPushString(outputArena, buffer, TRUE);
            }

            break;
        }

        case Op_add:
        {
            instruction_operand operand0 = instruction->Operands[0];
            instruction_operand operand1 = instruction->Operands[1];

            U16 oldValue = GetOperandValue(processor, operand0);
            U16 value0 = GetOperandValue(processor, operand0);
            U16 value1 = GetOperandValue(processor, operand1);
            U16 finalValue = value1 + value0;

            SetOperandValue(processor, &operand0, finalValue);
            SetRegisterFlag(processor, RegisterFlag_ZF, (finalValue == 0));

            // TODO (Aaron): Does the signed flag still get set if we are assigning to memory?
            if (operand0.Type == Operand_Register)
            {
                UpdateSignedRegisterFlag(processor, operand0.Register, finalValue);

                if (oldValue != finalValue)
                {
                    // printf(" %s:0x%x->0x%x",
                    //        GetRegisterMnemonic(operand0.Register),
                    //        value0,
                    //        finalValue);

                    snprintf(buffer, BUFFER_SIZE,
                             " %s:0x%x->0x%x",
                           GetRegisterMnemonic(operand0.Register),
                           value0,
                           finalValue);

                    ArenaCircularPushString(outputArena, buffer, TRUE);
                }
            }

            break;
        }

        case Op_sub:
        {
            instruction_operand operand0 = instruction->Operands[0];
            instruction_operand operand1 = instruction->Operands[1];

            U16 oldValue = GetOperandValue(processor, operand0);
            U16 value0 = GetOperandValue(processor, operand0);
            U16 value1 = GetOperandValue(processor, operand1);
            U16 finalValue = value0 - value1;

            SetOperandValue(processor, &operand0, finalValue);
            SetRegisterFlag(processor, RegisterFlag_ZF, (finalValue == 0));

            // TODO (Aaron): Does the signed flag still get set if we are assigning to memory?
            if (operand0.Type == Operand_Register)
            {
                UpdateSignedRegisterFlag(processor, operand0.Register, finalValue);

                if (oldValue != finalValue)
                {
                    // printf(" %s:0x%x->0x%x",
                    //        GetRegisterMnemonic(operand0.Register),
                    //        value0,
                    //        finalValue);

                    snprintf(buffer, BUFFER_SIZE,
                             " %s:0x%x->0x%x",
                           GetRegisterMnemonic(operand0.Register),
                           value0,
                           finalValue);

                    ArenaCircularPushString(outputArena, buffer, TRUE);
                }
            }

            break;
        }

        case Op_cmp:
        {
            instruction_operand operand0 = instruction->Operands[0];
            instruction_operand operand1 = instruction->Operands[1];

            U16 value0 = GetOperandValue(processor, operand0);
            U16 value1 = GetOperandValue(processor, operand1);
            U16 finalValue = value0 - value1;

            SetRegisterFlag(processor, RegisterFlag_ZF, (finalValue == 0));
            UpdateSignedRegisterFlag(processor, operand0.Register, finalValue);

            break;
        }

        case Op_jne:
        {
            // jump if not equal to zero
            if (!GetRegisterFlag(processor, RegisterFlag_ZF))
            {
                instruction_operand operand0 = instruction->Operands[0];
                S8 offset = (S8)(operand0.Immediate.Value & 0xff);

                // modify instruction pointer
                processor->IP += offset;
            }

            break;
        }

        case Op_loop:
        {
            // decrement CX register by 1
            U16 cx = GetRegisterValue(processor, Reg_cx);
            --cx;
            SetRegisterValue(processor, Reg_cx, cx);

            // jump if CX not equal to zero
            if (cx != 0)
            {
                instruction_operand operand0 = instruction->Operands[0];
                S8 offset = (S8)(operand0.Immediate.Value & 0xff);

                processor->IP += offset;
            }
            break;
        }

        case Op_ret:
        {
            // Note (Aaron): Not implemented. Halts execution.
            break;
        }

        default:
        {
            // printf("unsupported instruction");
            snprintf(buffer, BUFFER_SIZE, "unsupported instruction");
            ArenaCircularPushString(outputArena, buffer, TRUE);
        }
    }

    // printf(" ip:0x%x->0x%x", processor->PrevIP, processor->IP);
    snprintf(buffer, BUFFER_SIZE, " ip:0x%x->0x%x", processor->PrevIP, processor->IP);
    ArenaCircularPushString(outputArena, buffer, TRUE);

    PrintFlagDiffs(oldFlags, processor->Flags, outputArena);
    ArenaCircularPushString(outputArena, (char *)" ", FALSE);
}


// Note (Aaron): Resets the processor's execution state
global_function void ResetProcessorExecution(processor_8086 *processor)
{
    processor->IP = 0;
    processor->PrevIP = 0;
    MemorySet((U8 *)&processor->Registers, 0, sizeof(processor->Registers));
    processor->Flags = 0;
    processor->InstructionCount = 0;
    processor->TotalClockCount = 0;
}


global_function B32 HasProcessorFinishedExecution(processor_8086 *processor)
{
    B32 result = processor->IP >= processor->ProgramSize;
    return result;
}

