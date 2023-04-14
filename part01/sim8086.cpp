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

// #pragma clang diagnostic ignored "-Wnull-dereference"

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
    { Reg_sp, 4, true, 0xffff},
    { Reg_bp, 5, true, 0xffff},
    { Reg_si, 6, true, 0xffff},
    { Reg_di, 7, true, 0xffff},
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


// Note (Aaron): Reads the next N bytes of the instruction stream into an instruction's bits.
// Advances both the instruction bits pointer and the processor's instruction pointer.
static void ReadInstructionStream(processor_8086 *processor, instruction *instruction, uint8 byteCount)
{
    // Note (Aaron): Currently only support 8086 instructions that have a maximum of 6 bytes.
    // In practice, we never read this many bytes at once.
    assert(byteCount < 6);

    // error out if we attempt to read outside of program memory
    if ((processor->IP + byteCount) > processor->ProgramSize)
    {
        printf("ERROR: Attempted to read outside of program memory (%i)\n", (processor->IP + byteCount));
        printf("\tprogram size: %i\n", processor->ProgramSize);
        printf("\tip: 0x%x (%i)\n",
               processor->IP,
               processor->IP);
        printf("\tbyteCount: %i\n", byteCount);

        exit(1);
    }

    // load instruction bytes out of memory
    uint8 *readStartPtr = processor->Memory + processor->IP;
    MemoryCopy(instruction->Bits.BytePtr, readStartPtr, byteCount);

    // advance pointers and counters
    instruction->Bits.BytePtr += byteCount;
    processor->IP += byteCount;
    instruction->Bits.ByteCount++;
}


// Note (Aaron): Requires width, mod and rm to be decoded in the decode_context
static void DecodeRmStr(processor_8086 *processor, instruction *instruction, instruction_operand *operand)
{
    // TODO (Aaron): It's still a little awkward passing all of these parameters. Try passing in operand index instead?

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
            // TODO (Aaron): Casey said that this special case is always a 16-bit displacement in Q&A #5 (at 27m30s)
            if (instruction->WidthBit == 0)
            {
                uint8 *readStartPtr = instruction->Bits.BytePtr;
                ReadInstructionStream(processor, instruction, 1);
                operand->Memory.DirectAddress = (uint16)(*readStartPtr);
            }
            else
            {
                uint8 *readStartPtr = instruction->Bits.BytePtr;
                ReadInstructionStream(processor, instruction, 2);
                operand->Memory.DirectAddress = (uint16)(*(uint16 *)readStartPtr);
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
            uint8 *readStartPtr = instruction->Bits.BytePtr;
            ReadInstructionStream(processor, instruction, 1);
            operand->Memory.Displacement = (int16)(*(int8 *)readStartPtr);
        }
        else if (instruction->ModBits == 0b10)
        {
            uint8 *readStartPtr = instruction->Bits.BytePtr;
            ReadInstructionStream(processor, instruction, 2);
            operand->Memory.Displacement = (int16)(*(int16 *)readStartPtr);
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
    instruction instruction = {};

    // read initial instruction byte for parsing
    processor->ProgramInstCount++;
    ReadInstructionStream(processor, &instruction, 1);

    // parse initial instruction byte
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

        // decode reg string
        operandReg.Type = Operand_Register;
        operandReg.Register = RegMemTables[instruction.WidthBit][instruction.RegBits];

        // decode r/m string
        DecodeRmStr(processor, &instruction, &operandRm);

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

        // decode r/m string
        DecodeRmStr(processor, &instruction, &operandDest);

        // read data. guaranteed to be at least 8-bits.
        // TODO (Aaron): Is the result signed or unsigned?
        // TODO (Aaron): The way immediate values are stored is incorrect
        // This implementation produces errors with listing 41 (cmp al, ___)
        if (instruction.WidthBit == 0b0)
        {
            uint8 *readStartPtr = instruction.Bits.BytePtr;
            ReadInstructionStream(processor, &instruction, 1);
            operandSource.Immediate.Value = (uint16)(*readStartPtr);
        }
        else if (instruction.WidthBit == 0b1)
        {
            uint8 *readStartPtr = instruction.Bits.BytePtr;
            ReadInstructionStream(processor, &instruction, 2);
            operandSource.Immediate.Value = (uint16)(*readStartPtr);
        }
        // unhandled case
        else
        {
            assert(false);
        }

        // prepend width hint when value is being assigned to memory
        // TODO (Aaron): Do we prepend the width hint if we are writing to a direct address? (mod == 00 and rm == 110)
        bool prependWidth = ((instruction.ModBits == 0b0)
                             || (instruction.ModBits == 0b1)
                             || (instruction.ModBits == 0b10));

        if (prependWidth)
        {
            assert(operandDest.Type == Operand_Memory);

            // TODO (Aaron): Might need to move this up into DecodeRmStr()
            operandDest.Memory.Flags |= Memory_PrependWidth;
            if (instruction.WidthBit == 1)
            {
                operandDest.Memory.Flags |= Memory_IsWide;
            }
        }

        instruction.Operands[0] = operandDest;
        instruction.Operands[1] = operandSource;
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
            operandSource.Immediate.Value = *(uint8 *)(&instruction.Bits.Byte1);
        }
        else if (instruction.WidthBit == 0b1)
        {
            // read 16-bit data
            ReadInstructionStream(processor, &instruction, 2);
            operandSource.Immediate.Value = *(uint16 *)(&instruction.Bits.Byte1);
        }
        // unhandled case
        else
        {
            assert(false);
        }

        operandDest.Register = RegMemTables[instruction.WidthBit][instruction.RegBits];
        instruction.Operands[0] = operandDest;
        instruction.Operands[1] = operandSource;
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
        operandMemory.Memory.Flags |= Memory_DirectAddress;

        instruction.WidthBit = instruction.Bits.Byte0 & 0b1;
        if (instruction.WidthBit == 0)
        {
            ReadInstructionStream(processor, &instruction, 1);
            operandMemory.Memory.DirectAddress = (uint16)(*(&instruction.Bits.Byte1));
        }
        else if (instruction.WidthBit == 1)
        {
            ReadInstructionStream(processor, &instruction, 2);
            operandMemory.Memory.DirectAddress = (uint16)(*(uint16 *)(&instruction.Bits.Byte1));
        }
        // unhandled case
        else
        {
            assert(false);
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
        // unhandled case
        else
        {
            assert(false);
        }
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
        // unhandled case
        else
        {
            assert(false);
        }

        // decode reg string
        operandReg.Register = RegMemTables[instruction.WidthBit][instruction.RegBits];

        // decode r/m string
        DecodeRmStr(processor, &instruction, &operandRm);

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
        // unhandled case
        else
        {
            assert(false);
        }

        // decode r/m string
        DecodeRmStr(processor, &instruction, &operandDest);

        // TODO (Aaron): Casey encoded this as a table in his implementation
        //      - If an instruction has no s bit, assume it is 0
        // read data. guaranteed to be at least 8-bits.
        // TODO (Aaron): Move this into instruction_operand?
        // TODO (Aaron): Using a signed 32bit int here assumes the data value is signed.
        //  - Check to see how we determine this?
        if (instruction.SignBit == 0b0 && instruction.WidthBit == 0)
        {
            // read 8-bit unsigned
            uint8 *readStartPtr = instruction.Bits.BytePtr;
            ReadInstructionStream(processor, &instruction, 1);
            // TODO (Aaron): I don't think we need to to use an int32 here
            // Should be an int16?
            operandSource.Immediate.Value = (int32)(*(uint8 *)readStartPtr);
        }
        else if (instruction.SignBit == 0b0 && instruction.WidthBit == 1)
        {
            // read 16-bit unsigned
            uint8 *readStartPtr = instruction.Bits.BytePtr;
            ReadInstructionStream(processor, &instruction, 2);
            operandSource.Immediate.Value = (int32)(*(uint16 *)readStartPtr);
        }
        else if (instruction.SignBit == 0b1 && instruction.WidthBit == 0)
        {
            // read 8-bit signed
            uint8 *readStartPtr = instruction.Bits.BytePtr;
            ReadInstructionStream(processor, &instruction, 1);
            operandSource.Immediate.Value = (int32)(*(int8 *)readStartPtr);
        }
        else if (instruction.SignBit == 0b1 && instruction.WidthBit == 1)
        {
            // read 8-bits and sign-extend to 16-bits
            uint8 *readStartPtr = instruction.Bits.BytePtr;
            ReadInstructionStream(processor, &instruction, 1);
            operandSource.Immediate.Value = (int32)(*(int8 *)readStartPtr);
        }

        // prepend width hint when immediate is being assigned to memory
        // TODO (Aaron): Do we prepend the width hint if we are writing to a direct address? (mod == 00 and rm == 110)
        bool prependWidth = ((instruction.ModBits == 0b0)
                             || (instruction.ModBits == 0b1)
                             || (instruction.ModBits == 0b10));

        if (prependWidth)
        {
            assert(operandDest.Type == Operand_Memory);

            // TODO (Aaron): Might need to move this up into DecodeRmStr() so that we can be sure the flag is set only
            operandDest.Memory.Flags |= Memory_PrependWidth;
            if (instruction.WidthBit == 1)
            {
                operandDest.Memory.Flags |= Memory_IsWide;
            }
        }

        instruction.Operands[0] = operandDest;
        instruction.Operands[1] = operandSource;
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
        // unhandled case
        else
        {
            assert(false);
        }

        // read data
        // TODO (Aaron): Is the value supposed to be signed or unsigned?
        // How do we determine that?
        if (instruction.WidthBit == 0b0)
        {
            ReadInstructionStream(processor, &instruction, 1);
            operandSource.Immediate.Value = (uint16)(*(uint8 *)(&instruction.Bits.Byte1));
        }
        else if (instruction.WidthBit == 0b1)
        {
            ReadInstructionStream(processor, &instruction, 2);
            operandSource.Immediate.Value = (uint16)(*(uint16 *)(&instruction.Bits.Byte1));
        }

        operandDest.Register = (instruction.WidthBit == 0b0) ? Reg_al : Reg_ax;

        instruction.Operands[0] = operandDest;
        instruction.Operands[1] = operandSource;
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
        instruction.OpType = Op_unknown;
        char instructionStr[32] = "";

        // read 8-bit signed offset for jumps
        ReadInstructionStream(processor, &instruction, 1);
        int8 offset = *(int8 *)(&instruction.Bits.Byte1);

        instruction_operand operand0 = {};
        operand0.Type = Operand_Immediate;
        operand0.Immediate.Flags |= Immediate_IsJump;
        // Note (Aaron): A signed 8-bit value will need to be extracted from
        // the unsigned 16-bit ImmediateValue when instructions are executed.
        operand0.Immediate.Value = offset;
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
                // unhandled instruction
                assert(false);
        }
    }

    // unsupported instruction
    else
    {
        instruction.OpType = Op_unknown;
        instruction_operand unknown = {};
        instruction.Operands[0] = unknown;
        instruction.Operands[1] = unknown;
    }

    return instruction;
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
            result = operand.Immediate.Value;
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

    // Flags that are changing to 0
    if ((oldFlags & Register_CF) && !(newFlags & Register_CF)) { printf("C"); }
    if ((oldFlags & Register_PF) && !(newFlags & Register_PF)) { printf("P"); }
    if ((oldFlags & Register_AF) && !(newFlags & Register_AF)) { printf("A"); }
    if ((oldFlags & Register_ZF) && !(newFlags & Register_ZF)) { printf("Z"); }
    if ((oldFlags & Register_SF) && !(newFlags & Register_SF)) { printf("S"); }
    if ((oldFlags & Register_OF) && !(newFlags & Register_OF)) { printf("O"); }

    printf("->");

    // Flags that are changing to 1
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

    // TODO (Aaron): A lot of redundant code here
    //  - Re-write switch statement with if-statements?
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
                    printf("unsupported instruction");
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

        case Op_jne:
        {
            // jump if not equal to zero
            if (!GetRegisterFlag(processor, Register_ZF))
            {
                instruction_operand operand0 = instruction->Operands[0];
                int8 offset = (int8)(operand0.Immediate.Value & 0xff);

                // modify instruction pointer
                processor->IP += offset;
            }

            break;
        }

        default:
        {
            printf("unsupported instruction");
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
        instruction_operand operand = instruction->Operands[i];

        // skip empty operands
        if(operand.Type == Operand_None)
        {
            continue;
        }

        printf("%s", Separator);
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
                if (operand.Immediate.Flags & Immediate_IsJump)
                {
                    int8 offset = (int8)(operand.Immediate.Value & 0xff);

                    // Note (Aaron): Offset the value to accommodate a NASM syntax peculiarity.
                    // NASM expects an offset value from the start of the instruction rather than
                    // the end (which is how the instructions are encoded).
                    offset += instruction->Bits.ByteCount;

                    printf(offset >= 0 ? "$+%i" : "$%i", offset);
                    break;
                }

                printf("%i", operand.Immediate.Value);
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
    printf("\tip: %04x (%i)\n",
           processor->IP,
           processor->IP);

    // align flags with register print out
    printf("    ");
    PrintFlags(processor);
}


int main(int argc, char const *argv[])
{

#if SIM8086_SLOW
    // Note (Aaron): Ensure OperationMnemonics[] accommodates all operation_types
    assert(ArrayCount(OperationMnemonics) == Op_count);
#endif

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

    // TODO (Aaron): Should I assert anything here?
    //  - Feedback for empty program?

    printf("; %s:\n", filename);
    printf("bits 16\n");

    while (processor.IP < processor.ProgramSize)
    {
        instruction instruction = DecodeNextInstruction(&processor);
        PrintInstruction(&instruction);

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
        printf("\n");
    }

    return 0;
}
