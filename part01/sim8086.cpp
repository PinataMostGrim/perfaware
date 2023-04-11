#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sim8086.h"
#include "sim8086_mnemonics.h"
#include "sim8086_mnemonics.cpp"

// SIM8086_SLOW:
//     0 - No slow code allowed!
//     1 - Slow code welcome


static register_id RegMemTables[3][8]
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


// Note (Aaron): Order of registers must match the order defined in 'register_id'
// in order for lookup to work
static register_info RegisterLookup[21]
{
    { Reg_unknown, 0, 0x0},
    { Reg_al, 0, false, 0xff00},
    { Reg_cl, 2, false, 0xff00},
    { Reg_dl, 3, false, 0xff00},
    { Reg_bl, 1, false, 0xff00},
    { Reg_ah, 0, false, 0x00ff},
    { Reg_ch, 2, false, 0x00ff},
    { Reg_dh, 3, false, 0x00ff},
    { Reg_bh, 1, false, 0x00ff},
    { Reg_ax, 0, true, 0xffff},
    { Reg_cx, 2, true, 0xffff},
    { Reg_dx, 3, true, 0xffff},
    { Reg_bx, 1, true, 0xffff},
    { Reg_sp, 5, true, 0xffff},
    { Reg_bp, 6, true, 0xffff},
    { Reg_si, 7, true, 0xffff},
    { Reg_di, 8, true, 0xffff},
    // Note (Aaron): We currently only simulate non-memory operations.
    // { Reg_bx_si, 0, 0x0},
    // { Reg_bx_di, 0, 0x0},
    // { Reg_bp_si, 0, 0x0},
    // { Reg_bp_di, 0, 0x0},
};


static void *MemoryCopy(void *destPtr, void const *sourcePtr, size_t size)
{
    assert(size > 0);

    unsigned char *source = (unsigned char *)sourcePtr;
    unsigned char *dest = (unsigned char *)destPtr;
    while(size--) *dest++ = *source++;

    return destPtr;
}


static uint8 *GetMemoryReadPtr(processor_8086 *processor)
{
    uint8 *result = processor->Memory + processor->IP;
    return result;
}


static void DecodeRmStr(decode_context *context, processor_8086 *processor, instruction *instruction, instruction_operand *operand)
{
    // Note (Aaron): Requires width, mod and rm to be decoded

    // memory mode, no displacement
    if(instruction->ModBits == 0b0)
    {
        operand->Type = Operand_Memory;
        operand->Memory = {};

        // general case, memory mode no displacement
        if(instruction->RmBits != 0b110)
        {
            operand->Memory.Register = RegMemTables[2][instruction->RmBits];
        }
        // special case for direct address in memory mode with no displacement (R/M == 110)
        else
        {
            operand->Memory.Flags |= Memory_DirectAddress;

            // read direct address
            if (instruction->WidthBit == 0)
            {
                MemoryCopy(context->bufferPtr, GetMemoryReadPtr(processor), 1);
                processor->IP++;
                operand->Memory.DirectAddress = (uint16)(*context->bufferPtr);
                context->bufferPtr++;
            }
            else
            {
                MemoryCopy(context->bufferPtr, GetMemoryReadPtr(processor), 2);
                operand->Memory.DirectAddress = (uint16)(*(uint16 *)context->bufferPtr);
                processor->IP += 2;
                context->bufferPtr += 2;
            }
        }
    }

    // memory mode, 8-bit and 16-bit displacement
    else if ((instruction->ModBits == 0b1) || (instruction->ModBits == 0b10))
    {
        operand->Type = Operand_Memory;
        operand->Memory = {};
        operand->Memory.Flags |= Memory_HasDisplacement;

        // read displacement value
        if (instruction->ModBits == 0b1)
        {
            MemoryCopy(context->bufferPtr, GetMemoryReadPtr(processor), 1);
            processor->IP++;

            operand->Memory.Displacement = (int16)(*(int8 *)context->bufferPtr);
            context->bufferPtr++;
        }
        else if (instruction->ModBits == 0b10)
        {
            MemoryCopy(context->bufferPtr, GetMemoryReadPtr(processor), 2);
            operand->Memory.Displacement = (int16)(*(int16 *)context->bufferPtr);
            processor->IP += 2;
            context->bufferPtr += 2;
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


static instruction DecodeNextInstruction(processor_8086 *processor)
{
    processor->PrevIP = processor->IP;

    instruction result = {};
    decode_context context = {};

    // read initial instruction byte for parsing
    context.bufferPtr = context.buffer;
    MemoryCopy(context.bufferPtr++, GetMemoryReadPtr(processor), 1);
    processor->IP++;

    context.InstructionCount++;

    // parse initial instruction byte
    // mov instruction - register/memory to/from register (0b100010)
    if ((context.byte0 >> 2) == 0b100010)
    {
        result.OpType = Op_mov;

        instruction_operand operandReg = {};
        instruction_operand operandRm = {};

        // parse initial instruction byte
        result.DirectionBit = (context.byte0 >> 1) & 0b1;
        result.WidthBit = context.byte0 & 0b1;

        // read second instruction byte and parse it
        MemoryCopy(context.bufferPtr++, GetMemoryReadPtr(processor), 1);
        processor->IP++;

        result.ModBits = (context.byte1 >> 6);
        result.RegBits = (context.byte1 >> 3) & 0b111;
        result.RmBits = context.byte1 & 0b111;

        // decode reg string
        operandReg.Type = Operand_Register;
        operandReg.Register = RegMemTables[result.WidthBit][result.RegBits];

        // decode r/m string
        DecodeRmStr(&context, processor, &result, &operandRm);

        // set dest and source strings
        // destination is in RM field, source is in REG field
        if (result.DirectionBit == 0)
        {
            result.Operands[0] = operandRm;
            result.Operands[1] = operandReg;
        }
        // destination is in REG field, source is in RM field
        else
        {
            result.Operands[0] = operandReg;
            result.Operands[1] = operandRm;
        }
    }

    // mov instruction - immediate to register/memory (0b1100011)
    else if ((context.byte0 >> 1) == 0b1100011)
    {
        result.OpType = Op_mov;
        instruction_operand operandSource = {};
        instruction_operand operandDest = {};
        operandSource.Type = Operand_Immediate;

        result.WidthBit = context.byte0 & 0b1;

        // read second instruction byte and parse it
        MemoryCopy(context.bufferPtr++, GetMemoryReadPtr(processor), 1);
        processor->IP++;

        result.ModBits = (context.byte1 >> 6);
        result.RmBits = (context.byte1) & 0b111;

        // Note (Aaron): No reg in this instruction

        // decode r/m string
        DecodeRmStr(&context, processor, &result, &operandDest);

        // read data. guaranteed to be at least 8-bits.
        uint16 data = 0;
        if (result.WidthBit == 0b0)
        {
            MemoryCopy(context.bufferPtr, GetMemoryReadPtr(processor), 1);
            processor->IP++;
            data = (uint16)(*context.bufferPtr);
            operandSource.ImmediateValue = (uint16)(*context.bufferPtr);
            context.bufferPtr++;
        }
        else if (result.WidthBit == 0b1)
        {
            MemoryCopy(context.bufferPtr, GetMemoryReadPtr(processor), 2);
            data = (uint16)(*context.bufferPtr);
            operandSource.ImmediateValue = (uint16)(*context.bufferPtr);
            processor->IP += 2;
            context.bufferPtr += 2;
        }
        // unhandled case
        else
        {
            assert(false);
        }

        // prepend width hint when value is being assigned to memory
        bool prependWidth = ((result.ModBits == 0b0)
                             || (result.ModBits == 0b1)
                             || (result.ModBits == 0b10));

        if (prependWidth)
        {
            assert(operandDest.Type == Operand_Memory);

            operandDest.Memory.Flags |= Memory_PrependWidth;
            if (result.WidthBit == 1)
            {
                operandDest.Memory.Flags |= Memory_IsWide;
            }
        }

        result.Operands[0] = operandDest;
        result.Operands[1] = operandSource;
    }

    // mov instruction - immediate to register (0b1011)
    else if ((context.byte0 >> 4) == 0b1011)
    {
        result.OpType = Op_mov;
        instruction_operand operandSource = {};
        instruction_operand operandDest = {};
        operandSource.Type = Operand_Immediate;
        operandDest.Type = Operand_Register;

        // parse width and reg
        result.WidthBit = (context.byte0 >> 3) & (0b1);
        result.RegBits = context.byte0 & 0b111;

        if (result.WidthBit == 0b0)
        {
            // read 8-bit data
            MemoryCopy(context.bufferPtr, GetMemoryReadPtr(processor), 1);
            processor->IP++;
            operandSource.ImmediateValue = *(uint8 *)context.bufferPtr;
            context.bufferPtr++;
        }
        else if (result.WidthBit == 0b1)
        {
            // read 16-bit data
            MemoryCopy(context.bufferPtr, GetMemoryReadPtr(processor), 2);
            operandSource.ImmediateValue = *(uint16 *)context.bufferPtr;
            processor->IP += 2;
            context.bufferPtr += 2;
        }
        // unhandled case
        else
        {
            assert(false);
        }

        operandDest.Register = RegMemTables[result.WidthBit][result.RegBits];
        result.Operands[0] = operandDest;
        result.Operands[1] = operandSource;
    }

    // mov - memory to accumulator and accumulator to memory
    else if (((context.byte0 >> 1) == 0b1010000) || ((context.byte0 >> 1) == 0b1010001))
    {
        result.OpType = Op_mov;
        instruction_operand operandAccumulator = {};
        instruction_operand operandMemory = {};
        operandAccumulator.Type = Operand_Register;
        operandAccumulator.Register = Reg_ax;
        operandMemory.Type = Operand_Memory;
        operandMemory.Memory.Flags |= Memory_DirectAddress;

        result.WidthBit = context.byte0 & 0b1;
        if (result.WidthBit == 0)
        {
            MemoryCopy(context.bufferPtr, GetMemoryReadPtr(processor), 1);
            processor->IP++;
            operandMemory.Memory.DirectAddress = (uint16)(*context.bufferPtr);
            context.bufferPtr++;
        }
        else if (result.WidthBit == 1)
        {
            MemoryCopy(context.bufferPtr, GetMemoryReadPtr(processor), 2);
            operandMemory.Memory.DirectAddress = (uint16)(*(uint16 *)context.bufferPtr);
            processor->IP += 2;
            context.bufferPtr += 2;
        }
        // unhandled case
        else
        {
            assert(false);
        }

        if ((context.byte0 >> 1) == 0b1010000)
        {
            result.Operands[0] = operandAccumulator;
            result.Operands[1] = operandMemory;
        }
        else if ((context.byte0 >> 1) == 0b1010001)
        {
            result.Operands[0] = operandMemory;
            result.Operands[1] = operandAccumulator;
        }
        // unhandled case
        else
        {
            assert(false);
        }
    }

    // add / sub / cmp - reg/memory with register to either
    else if (((context.byte0 >> 2) == 0b000000)
        || ((context.byte0 >> 2) == 0b001010)
        || ((context.byte0 >> 2) == 0b001110))
    {
        instruction_operand operandReg = {};
        operandReg.Type = Operand_Register;
        instruction_operand operandRm = {};

        // decode direction and width
        result.DirectionBit = (context.byte0 >> 1) & 0b1;
        result.WidthBit = context.byte0 & 0b1;

        // decode mod, reg and r/m
        MemoryCopy(context.bufferPtr++, GetMemoryReadPtr(processor), 1);
        processor->IP++;

        result.ModBits = (context.byte1 >> 6) & 0b11;
        result.RegBits = (context.byte1 >> 3) & 0b111;
        result.RmBits = context.byte1 & 0b111;

        // decode instruction type
        // add = 0b000
        if (((context.byte0 >> 3) & 0b111) == 0b000)
        {
            result.OpType = Op_add;
        }
        // sub = 0b101
        else if (((context.byte0 >> 3) & 0b111) == 0b101)
        {
            result.OpType = Op_sub;
        }
        // cmp = 0b111
        else if (((context.byte0 >> 3) & 0b111) == 0b111)
        {
            result.OpType = Op_cmp;
        }
        // unhandled case
        else
        {
            assert(false);
        }

        // decode reg string
        operandReg.Register = RegMemTables[result.WidthBit][result.RegBits];

        // decode r/m string
        DecodeRmStr(&context, processor, &result, &operandRm);

        // set dest and source strings
        // destination is in RM field, source is in REG field
        if (result.DirectionBit == 0)
        {
            result.Operands[0] = operandRm;
            result.Operands[1] = operandReg;
        }
        // destination is in REG field, source is in RM field
        else
        {
            result.Operands[0] = operandReg;
            result.Operands[1] = operandRm;
        }
    }

    // add / sub / cmp - immediate to register/memory
    else if ((context.byte0 >> 2) == 0b100000)
    {
        instruction_operand operandSource = {};
        instruction_operand operandDest = {};
        operandSource.Type = Operand_Immediate;

        result.SignBit = (context.byte0 >> 1) & 0b1;
        result.WidthBit = context.byte0 & 0b1;

        // decode mod and r/m
        MemoryCopy(context.bufferPtr++, GetMemoryReadPtr(processor), 1);
        processor->IP++;

        result.ModBits = (context.byte1 >> 6) & 0b11;
        result.RmBits = context.byte1 & 0b111;

        // decode instruction type
        // add = 0b000
        if (((context.byte1 >> 3) & 0b111) == 0b000)
        {
            result.OpType = Op_add;
        }
        // sub = 0b101
        else if (((context.byte1 >> 3) & 0b111) == 0b101)
        {
            result.OpType = Op_sub;
        }
        // cmp = 0b111
        else if (((context.byte1 >> 3) & 0b111) == 0b111)
        {
            result.OpType = Op_cmp;
        }
        // unhandled case
        else
        {
            assert(false);
        }

        // decode r/m string
        DecodeRmStr(&context, processor, &result, &operandDest);

        // read data. guaranteed to be at least 8-bits.
        int32 data = 0;
        if (result.SignBit == 0b0 && result.WidthBit == 0)
        {
            // read 8-bit unsigned
            MemoryCopy(context.bufferPtr, GetMemoryReadPtr(processor), 1);
            processor->IP++;
            data = (int32)(*(uint8 *)context.bufferPtr);
            operandSource.ImmediateValue = (int32)(*(uint8 *)context.bufferPtr);
            context.bufferPtr++;
        }
        else if (result.SignBit == 0b0 && result.WidthBit == 1)
        {
            // read 16-bit unsigned
            MemoryCopy(context.bufferPtr, GetMemoryReadPtr(processor), 2);
            data = (int32)(*(uint16 *)context.bufferPtr);
            operandSource.ImmediateValue = (int32)(*(uint16 *)context.bufferPtr);
            processor->IP += 2;
            context.bufferPtr += 2;
        }
        else if (result.SignBit == 0b1 && result.WidthBit == 0)
        {
            // read 8-bit signed
            MemoryCopy(context.bufferPtr, GetMemoryReadPtr(processor), 1);
            processor->IP++;
            data = (int32)(*(int8 *)context.bufferPtr);
            operandSource.ImmediateValue = (int32)(*(int8 *)context.bufferPtr);
            context.bufferPtr++;
        }
        else if (result.SignBit == 0b1 && result.WidthBit == 1)
        {
            // read 8-bits and sign-extend to 16-bits
            MemoryCopy(context.bufferPtr, GetMemoryReadPtr(processor), 1);
            processor->IP++;
            data = (int32)(*(int8 *)context.bufferPtr);
            operandSource.ImmediateValue = (int32)(*(int8 *)context.bufferPtr);
            context.bufferPtr++;
        }

        // prepend width hint when immediate is being assigned to memory
        bool prependWidth = ((result.ModBits == 0b0)
                             || (result.ModBits == 0b1)
                             || (result.ModBits == 0b10));

        if (prependWidth)
        {
            assert(operandDest.Type == Operand_Memory);

            operandDest.Memory.Flags |= Memory_PrependWidth;
            if (result.WidthBit == 1)
            {
                operandDest.Memory.Flags |= Memory_IsWide;
            }
        }

        result.Operands[0] = operandDest;
        result.Operands[1] = operandSource;
    }

    // add / sub / cmp - immediate to/from/with accumulator
    else if (((context.byte0 >> 1) == 0b0000010)
        || ((context.byte0 >> 1) == 0b0010110)
        || ((context.byte0 >> 1) == 0b0011110))
    {
        result.WidthBit = context.byte0 & 0b1;

        instruction_operand operandSource = {};
        instruction_operand operandDest = {};
        operandSource.Type = Operand_Immediate;
        operandDest.Type = Operand_Register;

        // decode instruction type
        // add = 0b000
        if (((context.byte0 >> 3) & 0b111) == 0b000)
        {
            result.OpType = Op_add;
        }
        // sub = 0b101
        else if (((context.byte0 >> 3) & 0b111) == 0b101)
        {
            result.OpType = Op_sub;
        }
        // cmp = 0b111
        else if (((context.byte0 >> 3) & 0b111) == 0b111)
        {
            result.OpType = Op_cmp;
        }
        // unhandled case
        else
        {
            assert(false);
        }

        // read data
        uint16 data = 0;
        if (result.WidthBit == 0b0)
        {
            MemoryCopy(context.bufferPtr, GetMemoryReadPtr(processor), 1);
            processor->IP++;
            data = (uint16)(*(uint8 *)context.bufferPtr);
            operandSource.ImmediateValue = (uint16)(*(uint8 *)context.bufferPtr);
            context.bufferPtr++;
        }
        else if (result.WidthBit == 0b1)
        {
            MemoryCopy(context.bufferPtr, GetMemoryReadPtr(processor), 2);
            data = (uint16)(*(uint16 *)context.bufferPtr);
            operandSource.ImmediateValue = (uint16)(*(uint16 *)context.bufferPtr);
            processor->IP += 2;
            context.bufferPtr += 2;
        }

        operandDest.Register = (result.WidthBit == 0b0) ? Reg_al : Reg_ax;

        result.Operands[0] = operandDest;
        result.Operands[1] = operandSource;
    }

    // control transfer instructions
    else if ((context.byte0 == 0b01110101)     // jnz / jne
             || (context.byte0 == 0b01110100)  // je
             || (context.byte0 == 0b01111100)  // jl
             || (context.byte0 == 0b01111110)  // jle
             || (context.byte0 == 0b01110010)  // jb
             || (context.byte0 == 0b01110110)  // jbe
             || (context.byte0 == 0b01111010)  // jp
             || (context.byte0 == 0b01110000)  // jo
             || (context.byte0 == 0b01111000)  // js
             || (context.byte0 == 0b01111101)  // jnl
             || (context.byte0 == 0b01111111)  // jg
             || (context.byte0 == 0b01110011)  // jnb
             || (context.byte0 == 0b01110111)  // ja
             || (context.byte0 == 0b01111011)  // jnp
             || (context.byte0 == 0b01110001)  // jno
             || (context.byte0 == 0b01111001)  // jns
             || (context.byte0 == 0b11100010)  // loop
             || (context.byte0 == 0b11100001)  // loopz
             || (context.byte0 == 0b11100000)  // loopnz
             || (context.byte0 == 0b11100011)) // jcxz
    {
        result.OpType = Op_jmp;
        char instructionStr[32] = "";

        switch(context.byte0)
        {
            // jnz / jne
            case 0b01110101:
                sprintf(instructionStr, "jnz");
                break;

            // je
            case 0b01110100:
                sprintf(instructionStr, "je");
                break;

            // jl
            case 0b01111100:
                sprintf(instructionStr, "jl");
                break;

            // jle
            case 0b01111110:
                sprintf(instructionStr, "jle");
                break;

            // jb
            case 0b01110010:
                sprintf(instructionStr, "jb");
                break;

            // jbe
            case 0b01110110:
                sprintf(instructionStr, "jbe");
                break;

            // jp
            case 0b01111010:
                sprintf(instructionStr, "jp");
                break;

            // jo
            case 0b01110000:
                sprintf(instructionStr, "jo");
                break;

            // js
            case 0b01111000:
                sprintf(instructionStr, "js");
                break;

            // jnl
            case 0b01111101:
                sprintf(instructionStr, "jnl");
                break;

            // jg
            case 0b01111111:
                sprintf(instructionStr, "jg");
                break;

            // jnb
            case 0b01110011:
                sprintf(instructionStr, "jnb");
                break;

            // ja
            case 0b01110111:
                sprintf(instructionStr, "ja");
                break;

            // jnp
            case 0b01111011:
                sprintf(instructionStr, "jnp");
                break;

            // jno
            case 0b01110001:
                sprintf(instructionStr, "jno");
                break;

            // jns
            case 0b01111001:
                sprintf(instructionStr, "jns");
                break;

            // loop
            case 0b11100010:
                sprintf(instructionStr, "LOOP");
                break;

            // loopz
            case 0b11100001:
                sprintf(instructionStr, "LOOPZ");
                break;

            // loopnz
            case 0b11100000:
                sprintf(instructionStr, "LOOPNZ");
                break;

            // jcxz
            case 0b11100011:
                sprintf(instructionStr, "JCXZ");
                break;
            default:
                // unhandled instruction
                assert(false);
        }

        // read 8-bit signed offset
        MemoryCopy(context.bufferPtr, GetMemoryReadPtr(processor), 1);
        processor->IP++;
        int8 offset = *(int8 *)context.bufferPtr;
        context.bufferPtr++;

        printf("%s %i\n", instructionStr, offset);
    }

    // unsupported instruction
    else
    {
        // Note (Aaron): Unsupported instruction
        result.OpType = Op_unknown;
        instruction_operand unknown = {};
        result.Operands[0] = unknown;
        result.Operands[1] = unknown;
    }

    return result;
}


uint16 GetRegister(processor_8086 *processor, register_id targetRegister)
{
    uint16 result = 0;

    register_info info = RegisterLookup[targetRegister];
    result = (processor->Registers[info.RegisterIndex] & info.Mask);

    return result;
}


void SetRegister(processor_8086 *processor, register_id targetRegister, uint16 value)
{
    register_info info = RegisterLookup[targetRegister];
    processor->Registers[info.RegisterIndex] = (value & info.Mask);
}


uint8 GetRegisterFlag(processor_8086 *processor, register_flags flag)
{
    return (processor->Flags & flag);
}


void SetRegisterFlag(processor_8086 *processor, register_flags flag, bool set)
{
    if (set)
    {
        processor->Flags |= flag;
        return;
    }

    processor->Flags &= ~flag;
}


void UpdateSignedRegisterFlag(processor_8086 *processor, register_id targetRegister, uint16 value)
{
    register_info info = RegisterLookup[targetRegister];
    if (info.IsWide)
    {
        // use 16 bit mask
        SetRegisterFlag(processor, Register_SF, (value >> 15) == 1);
    }
    else
    {
        // use 8 bit mask
        SetRegisterFlag(processor, Register_SF, (value >> 7) == 1);
    }
}


uint16 GetOperandValue(processor_8086 *processor, instruction_operand operand)
{
    uint16 result = 0;
    switch (operand.Type)
    {
        case Operand_Register:
        {
            result = GetRegister(processor, operand.Register);
            break;
        }
        case Operand_Immediate:
        {
            result = operand.ImmediateValue;
            break;
        }
        case Operand_Memory:
        {
            // Note (Aaron): Unsupported
            break;
        }
        default:
        {
            // Note (Aaron): Invalid instruction
            assert(false);
            break;
        }
    }

    return result;
}


void PrintFlags(processor_8086 *processor, bool force = false)
{
    if (processor->Flags == 0 && !force)
    {
        return;
    }

    printf(" flags:->");

    if (processor->Flags & Register_CF) { printf("C"); }
    if (processor->Flags & Register_PF) { printf("P"); }
    if (processor->Flags & Register_AF) { printf("A"); }
    if (processor->Flags & Register_ZF) { printf("Z"); }
    if (processor->Flags & Register_SF) { printf("S"); }
    if (processor->Flags & Register_OF) { printf("O"); }
}


void PrintFlagDiffs(uint8 oldFlags, uint8 newFlags)
{
    if (oldFlags == newFlags)
    {
        return;
    }

    printf(" flags:");

    // loop twice
    if ((oldFlags & Register_CF) && !(newFlags & Register_CF)) { printf("C"); }
    if ((oldFlags & Register_PF) && !(newFlags & Register_PF)) { printf("P"); }
    if ((oldFlags & Register_AF) && !(newFlags & Register_AF)) { printf("A"); }
    if ((oldFlags & Register_ZF) && !(newFlags & Register_ZF)) { printf("Z"); }
    if ((oldFlags & Register_SF) && !(newFlags & Register_SF)) { printf("S"); }
    if ((oldFlags & Register_OF) && !(newFlags & Register_OF)) { printf("O"); }

    printf("->");

    if (!(oldFlags & Register_CF) && (newFlags & Register_CF)) { printf("C"); }
    if (!(oldFlags & Register_PF) && (newFlags & Register_PF)) { printf("P"); }
    if (!(oldFlags & Register_AF) && (newFlags & Register_AF)) { printf("A"); }
    if (!(oldFlags & Register_ZF) && (newFlags & Register_ZF)) { printf("Z"); }
    if (!(oldFlags & Register_SF) && (newFlags & Register_SF)) { printf("S"); }
    if (!(oldFlags & Register_OF) && (newFlags & Register_OF)) { printf("O"); }
}


void ExecuteInstruction(processor_8086 *processor, instruction *instruction)
{
    uint8 oldFlags = processor->Flags;

    switch (instruction->OpType)
    {
        case Op_mov:
        {
            instruction_operand operand0 = instruction->Operands[0];
            instruction_operand operand1 = instruction->Operands[1];

            uint16 sourceValue = GetOperandValue(processor, operand1);

            switch (operand0.Type)
            {
                case Operand_Register:
                {
                    uint16 destValue = GetRegister(processor, operand0.Register);
                    SetRegister(processor, operand0.Register, sourceValue);

                    // Note (Aaron): mov does not set the zero flag or the signed flag

                    printf("%s:0x%x->0x%x",
                           GetRegisterMnemonic(operand0.Register),
                           destValue,
                           sourceValue);

                    break;
                }
                case Operand_Memory:
                {
                    // Note (Aaron): Currently unsupported
                    printf("unsupported instruction ");
                    break;
                }
                case Operand_Immediate:
                default:
                {
                    // Note (Aaron): Invalid instruction
                    assert(false);
                    break;
                }
            }

            break;
        }

        case Op_add:
        {
            instruction_operand operand0 = instruction->Operands[0];
            instruction_operand operand1 = instruction->Operands[1];

            uint16 value0 = GetOperandValue(processor, operand0);
            uint16 value1 = GetOperandValue(processor, operand1);
            uint16 finalValue = value1 + value0;

            SetRegister(processor, operand0.Register, finalValue);
            SetRegisterFlag(processor, Register_ZF, (finalValue == 0));
            UpdateSignedRegisterFlag(processor, operand0.Register, finalValue);

            printf("%s:0x%x->0x%x",
                   GetRegisterMnemonic(operand0.Register),
                   value0,
                   finalValue);

            break;
        }

        case Op_sub:
        {
            instruction_operand operand0 = instruction->Operands[0];
            instruction_operand operand1 = instruction->Operands[1];

            uint16 value0 = GetOperandValue(processor, operand0);
            uint16 value1 = GetOperandValue(processor, operand1);
            uint16 finalValue = value0 - value1;

            SetRegister(processor, operand0.Register, finalValue);
            SetRegisterFlag(processor, Register_ZF, (finalValue == 0));
            UpdateSignedRegisterFlag(processor, operand0.Register, finalValue);

            printf("%s:0x%x->0x%x",
                   GetRegisterMnemonic(operand0.Register),
                   value0,
                   finalValue);

            break;
        }

        case Op_cmp:
        {
            instruction_operand operand0 = instruction->Operands[0];
            instruction_operand operand1 = instruction->Operands[1];

            uint16 value0 = GetOperandValue(processor, operand0);
            uint16 value1 = GetOperandValue(processor, operand1);
            uint16 finalValue = value0 - value1;

            SetRegisterFlag(processor, Register_ZF, (finalValue == 0));
            UpdateSignedRegisterFlag(processor, operand0.Register, finalValue);

            break;
        }

        default:
        {
            printf("unsupported instruction ");
        }
    }

    printf(" ip:0x%x->0x%x", processor->PrevIP, processor->IP);
    PrintFlagDiffs(oldFlags, processor->Flags);
}


static void PrintInstruction(instruction *instruction)
{
    printf("%s ", GetOpMnemonic(instruction->OpType));
    const char *Separator = "";

    for (int i = 0; i < ArrayCount(instruction->Operands); ++i)
    {
        printf("%s", Separator);
        Separator = ", ";

        instruction_operand operand = instruction->Operands[i];
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
                    printf("%s ", (operand.Memory.Flags & Memory_IsWide) ? "word": "byte");
                }

                // print direct address
                if (operand.Memory.Flags & Memory_DirectAddress)
                {
                    printf("[%i]", operand.Memory.DirectAddress);
                    break;
                }

                // print memory with optional displacement
                printf("[");
                printf("%s", GetRegisterMnemonic(operand.Memory.Register));

                if (operand.Memory.Flags & Memory_HasDisplacement)
                {
                    if (operand.Memory.Displacement >= 0)
                    {
                        printf(" + %i", operand.Memory.Displacement);
                    }
                    else
                    {
                        printf(" - %i", operand.Memory.Displacement * -1);
                    }
                }

                printf("]");

                break;
            }
            case Operand_Register:
            {
                printf("%s", GetRegisterMnemonic(operand.Register));
                break;
            }
            case Operand_Immediate:
            {
                printf("%i", operand.ImmediateValue);
                break;
            }
            default:
            {
                printf("?");
            }
        }
    }
}


static void PrintRegisters(processor_8086 *processor)
{
    register_id toDisplay[] =
    {
        Reg_ax,
        Reg_bx,
        Reg_cx,
        Reg_dx,
        Reg_sp,
        Reg_bp,
        Reg_si,
        Reg_di,
    };

    printf("Registers:\n");

    for (int i = 0; i < ArrayCount(toDisplay); ++i)
    {
        uint16 value = GetRegister(processor, toDisplay[i]);

        // Reduce noise by omitting registers with a value of 0
        if (value == 0)
        {
            continue;
        }

        printf("\t%s: %04x (%i)\n",
               GetRegisterMnemonic(toDisplay[i]),
               value,
               value);
    }

    // print instruction pointer
    printf("\t%s: %04x (%i)\n",
           "ip",
           processor->IP,
           processor->IP);

    // align flags with register print out
    printf("    ");
    PrintFlags(processor);
}


int main(int argc, char const *argv[])
{
    if (argc < 2 ||  argc > 3)
    {
        printf("usage: sim8086 [--exec] filename\n\n");
        printf("disassembles 8086/88 assembly\n\n");

        printf("positional arguments:\n");
        printf("  filename\t\tassembly file to load\n");
        printf("\n");

        printf("options:\n");
        printf("  --exec, -e\t\tsimulate execution of assembly\n");

        exit(1);
    }

    // process command line arguments
    bool simulateInstructions = false;
    const char *filename = "";

    for (int i = 1; i < argc; ++i)
    {
        if ((strncmp("--exec", argv[i], 6) == 0)
            || (strncmp("-e", argv[i], 2) == 0))
        {
            simulateInstructions = true;
            continue;
        }

        filename = argv[i];
    }

    // initialize processor
    processor_8086 processor = {};
    processor.Memory = (uint8 *)malloc(processor.MemoryMaxSize);
    if (!processor.Memory)
    {
        printf("ERROR: Unable to allocate main memory for 8086\n");
        exit(1);
    }

    FILE *file = {};
    file = fopen(filename, "rb");

    if(!file)
    {
        printf("Unable to open '%s'\n", filename);
        exit(1);
    }

    processor.ProgramSize = (uint16)fread(processor.Memory, 1, processor.MemoryMaxSize, file);

    // error handling for file read
    if (ferror(file))
    {
        printf("ERROR:Encountered error while reading file '%s'", filename);
        exit(1);
    }

    if (!feof(file))
    {
        printf("ERROR:Program size exceeds processor memory; unable to load\n\n");
        exit(1);
    }

    fclose(file);

    printf("; %s:\n", filename);
    printf("bits 16\n");

    while (processor.IP < processor.ProgramSize)
    {
        instruction instruction = DecodeNextInstruction(&processor);

        if (instruction.OpType != Op_jmp)
        {
            PrintInstruction(&instruction);
        }

        if (simulateInstructions)
        {
            printf(" ; ");
            ExecuteInstruction(&processor, &instruction);
        }

        printf("\n");
    }

    if (simulateInstructions)
    {
        printf("\n");
        PrintRegisters(&processor);
    }

    printf("\n");

    return 0;
}
