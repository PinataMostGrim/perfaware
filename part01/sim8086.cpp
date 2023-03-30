#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define internal static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;


// SIM8086_SLOW:
//     0 - No slow code allowed!
//     1 - Slow code welcome


struct field_encoding
{
    const char *table[8] {};
};

global_variable field_encoding RegisterEncodings[2]
{
    {
        "al",
        "cl",
        "dl",
        "bl",
        "ah",
        "ch",
        "dh",
        "bh",
    },
    {
        "ax",
        "cx",
        "dx",
        "bx",
        "sp",
        "bp",
        "si",
        "di",
    },
};

global_variable field_encoding RegMemEncodings
{
    {
        "bx + si",
        "bx + di",
        "bp + si",
        "bp + di",
        "si",
        "di",
        "bp",
        "bx",
    }
};

typedef struct instruction_data
{
    FILE *file = {};

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
    uint32 instructionCount = 0;

    uint8 direction = 0;
    uint8 width = 0;
    uint8 mod = 0;
    uint8 reg = 0;
    uint8 rm = 0;
    uint16 address = 0;
    int16 displacement = 0;

    char regStr[32] = "";
    char rmStr[32] = "";
    char displacementStr[32] = "";

    char instructionStr[32] = "";
    char destStr[32] = "";
    char sourceStr[32] = "";
    char result[256] = "";

} instructioninstruction_data;

#if SIM8086_SLOW
internal void
ClearInstructionData(instruction_data *instructions)
{
    memset(instructions->buffer, 0, 6);

    instructions->direction = 0;
    instructions->width = 0;
    instructions->mod = 0;
    instructions->reg = 0;
    instructions->rm = 0;
    instructions->address = 0;
    instructions->displacement = 0;

    sprintf(instructions->regStr, "");
    sprintf(instructions->rmStr, "");
    sprintf(instructions->displacementStr, "");

    sprintf(instructions->instructionStr, "");
    sprintf(instructions->destStr, "");
    sprintf(instructions->sourceStr, "");
}
#endif


internal void
DecodeRmStr(instruction_data *instructions)
{
    // Note (Aaron): Requires width, mod and rm to be decoded

    // memory mode, no displacement
    if(instructions->mod == 0b0)
    {
        // special case for direct address in memory mode with no displacement (R/M == 110)
        if(instructions->rm == 0b110)
        {
            // read direct address
            if (instructions->width == 0)
            {
                fread(instructions->bufferPtr, 1, 1, instructions->file);
                instructions->address = (uint16)(*instructions->bufferPtr);
                instructions->bufferPtr++;
            }
            else
            {
                fread(instructions->bufferPtr, 1, 2, instructions->file);
                instructions->address = (uint16)(*(uint16 *)instructions->bufferPtr);
                instructions->bufferPtr += 2;
            }

            sprintf(instructions->rmStr, "[%i]", instructions->address);
        }
        else
        {
            sprintf(instructions->rmStr, "[%s]", RegMemEncodings.table[instructions->rm]);
        }
    }
    // memory mode, 8-bit and 16-bit displacement
    else if ((instructions->mod == 0b1) || (instructions->mod == 0b10))
    {
        // read displacement value
        if (instructions->mod == 0b1)
        {
            fread(instructions->bufferPtr, 1, 1, instructions->file);
            instructions->displacement = (int16)(*(int8 *)instructions->bufferPtr);
            instructions->bufferPtr++;
        }
        else if (instructions->mod == 0b10)
        {
            fread(instructions->bufferPtr, 1, 2, instructions->file);
            instructions->displacement = (int16)(*(int16 *)instructions->bufferPtr);
            instructions->bufferPtr += 2;
        }

        if (instructions->displacement < 0)
        {
            sprintf(instructions->displacementStr, " - %i", -1 * instructions->displacement);
        }
        else if (instructions->displacement > 0)
        {
            sprintf(instructions->displacementStr, " + %i", instructions->displacement);
        }

        sprintf(instructions->rmStr, "[%s%s]", RegMemEncodings.table[instructions->rm], instructions->displacementStr);
    }
    // register mode, no displacement
    else if (instructions->mod == 0b11)
    {
        sprintf(instructions->rmStr, "%s", RegisterEncodings[instructions->width].table[instructions->rm]);
    }
}

int main(int argc, char const *argv[])
{
    if (argc > 2)
    {
        printf("usage: sim8086 filename\n\n");
        printf("disassembles 8086/88 assembly\n\n");
        printf("positional arguments:\n");
        printf("  filename\t\tassembly file to load\n");

        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    instruction_data instructions = {};
    instructions.file = fopen(filename, "rb");
    size_t bytesRead;

    if(!instructions.file)
    {
        printf("Unable to open '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    printf("; %s:\n", filename);
    printf("bits 16\n");

    // read initial instruction byte for parsing
    bytesRead = fread(instructions.bufferPtr, 1, 1, instructions.file);
    instructions.bufferPtr++;
    instructions.instructionCount++;

    // main loop
    while(bytesRead)
    {
        // parse initial instruction byte
        // mov instruction - register/memory to/from register (0b100010)
        if ((instructions.byte0 >> 2) == 0b100010)
        {
            sprintf(instructions.instructionStr, "mov");

            // parse initial instruction byte
            instructions.direction = (instructions.byte0 >> 1) & 0b1;
            instructions.width = instructions.byte0 & 0b1;

            // read second instruction byte and parse it
            fread(instructions.bufferPtr, 1, 1, instructions.file);
            instructions.bufferPtr++;

            instructions.mod = (instructions.byte1 >> 6);
            instructions.reg = (instructions.byte1 >> 3) & 0b111;
            instructions.rm = instructions.byte1 & 0b111;

            // decode reg string
            sprintf(instructions.regStr, "%s", RegisterEncodings[instructions.width].table[instructions.reg]);

            // decode r/m string
            DecodeRmStr(&instructions);

            // set dest and source strings
            // destination is in RM field, source is in REG field
            if (instructions.direction == 0)
            {
                sprintf(instructions.destStr, "%s", instructions.rmStr);
                sprintf(instructions.sourceStr, "%s", instructions.regStr);

            }
            // destination is in REG field, source is in RM field
            else
            {
                sprintf(instructions.destStr, "%s", instructions.regStr);
                sprintf(instructions.sourceStr, "%s", instructions.rmStr);
            }

            printf("%s %s, %s\n", instructions.instructionStr, instructions.destStr, instructions.sourceStr);
        }
        // mov instruction - immediate to register/memory (0b1100011)
        else if ((instructions.byte0 >> 1) == 0b1100011)
        {
            sprintf(instructions.instructionStr, "mov");
            instructions.width = instructions.byte0 & 0b1;

            // read second instruction byte and parse it
            fread(instructions.bufferPtr, 1, 1, instructions.file);
            instructions.bufferPtr++;

            instructions.mod = (instructions.byte1 >> 6);
            instructions.rm = (instructions.byte1) & 0b111;

            // Note (Aaron): No reg in this instruction

            // decode r/m string
            DecodeRmStr(&instructions);

            // read data. guaranteed to be at least 8-bits.
            uint16 data = 0;
            if (instructions.width == 0b0)
            {
                fread(instructions.bufferPtr, 1, 1, instructions.file);
                data = (uint16)(*instructions.bufferPtr);
                instructions.bufferPtr++;
            }
            else if (instructions.width == 0b1)
            {
                fread(instructions.bufferPtr, 1, 2, instructions.file);
                data = (uint16)(*(uint16 *)instructions.bufferPtr);
                instructions.bufferPtr += 2;
            }
            // unhandled case
            else
            {
                assert(false);
            }

            // prepend width hint when value is being assigned to memory
            bool prependWidth = ((instructions.mod == 0b0)
                                 || (instructions.mod == 0b1)
                                 || (instructions.mod == 0b10));

            if (prependWidth)
            {
                sprintf(instructions.destStr,
                        "%s %s",
                        ((instructions.width == 0) ? "byte" : "word"),
                        instructions.rmStr);
            }
            else
            {
                sprintf(instructions.destStr, "%s", instructions.rmStr);
            }
            sprintf(instructions.sourceStr, "%i", data);

            printf("%s %s, %s\n", instructions.instructionStr, instructions.destStr, instructions.sourceStr);
        }
        // mov instruction - immediate to register (0b1011)
        else if ((instructions.byte0 >> 4) == 0b1011)
        {
            sprintf(instructions.instructionStr, "mov");

            // parse width and reg
            instructions.width = (instructions.byte0 >> 3) & (0b1);
            instructions.reg = instructions.byte0 & 0b111;

            if (instructions.width == 0b0)
            {
                // read 8-bit data
                fread(instructions.bufferPtr, 1, 1, instructions.file);
                sprintf(instructions.sourceStr, "%i", *(uint8 *)instructions.bufferPtr);
                instructions.bufferPtr++;
            }
            else if (instructions.width == 0b1)
            {
                // read 16-bit data
                fread(instructions.bufferPtr, 1, 2, instructions.file);
                sprintf(instructions.sourceStr, "%i", *(uint16 *)instructions.bufferPtr);
                instructions.bufferPtr += 2;
            }
            // unhandled case
            else
            {
                assert(false);
            }

            sprintf(instructions.destStr, "%s", RegisterEncodings[instructions.width].table[instructions.reg]);

            printf("%s %s, %s\n", instructions.instructionStr, instructions.destStr, instructions.sourceStr);
        }
        // mov - memory to accumulator and accumulator to memory
        else if (((instructions.byte0 >> 1) == 0b1010000) || ((instructions.byte0 >> 1) == 0b1010001))
        {
            sprintf(instructions.instructionStr, "mov");

            instructions.width = instructions.byte0 & 0b1;
            if (instructions.width == 0)
            {
                fread(instructions.bufferPtr, 1, 1, instructions.file);
                instructions.address = (uint16)(*instructions.bufferPtr);
                instructions.bufferPtr++;
            }
            else if (instructions.width == 1)
            {
                fread(instructions.bufferPtr, 1, 2, instructions.file);
                instructions.address = (uint16)(*(uint16 *)instructions.bufferPtr);
                instructions.bufferPtr++;
            }
            // unhandled case
            else
            {
                assert(false);
            }

            if ((instructions.byte0 >> 1) == 0b1010000)
            {
                sprintf(instructions.destStr, "%s", "ax");
                sprintf(instructions.sourceStr, "[%i]", instructions.address);
            }
            else if ((instructions.byte0 >> 1) == 0b1010001)
            {
                sprintf(instructions.destStr, "[%i]", instructions.address);
                sprintf(instructions.sourceStr, "%s", "ax");
            }
            // unhandled case
            else
            {
                assert(false);
            }

            printf("%s %s, %s\n", instructions.instructionStr, instructions.destStr, instructions.sourceStr);
        }
        // add / sub / cmp - reg/memory with register to either
        else if (((instructions.byte0 >> 2) == 0b000000)
            || ((instructions.byte0 >> 2) == 0b001010)
            || ((instructions.byte0 >> 2) == 0b001110))
        {
            // decode direction and width
            instructions.direction = (instructions.byte0 >> 1) & 0b1;
            instructions.width = instructions.byte0 & 0b1;

            // decode mod, reg and r/m
            fread(instructions.bufferPtr, 1, 1, instructions.file);
            instructions.bufferPtr++;

            instructions.mod = (instructions.byte1 >> 6) & 0b11;
            instructions.reg = (instructions.byte1 >> 3) & 0b111;
            instructions.rm = instructions.byte1 & 0b111;

            // decode instruction type
            // add = 0b000
            if (((instructions.byte0 >> 3) & 0b111) == 0b000)
            {
                sprintf(instructions.instructionStr, "add");
            }
            // sub = 0b101
            else if (((instructions.byte0 >> 3) & 0b111) == 0b101)
            {
                sprintf(instructions.instructionStr, "sub");
            }
            // cmp = 0b111
            else if (((instructions.byte0 >> 3) & 0b111) == 0b111)
            {
                sprintf(instructions.instructionStr, "cmp");
            }
            // unhandled case
            else
            {
                assert(false);
            }

            // decode reg string
            sprintf(instructions.regStr, "%s", RegisterEncodings[instructions.width].table[instructions.reg]);

            // decode r/m string
            DecodeRmStr(&instructions);

            // set dest and source strings
            // destination is in RM field, source is in REG field
            if (instructions.direction == 0)
            {
                sprintf(instructions.destStr, "%s", instructions.rmStr);
                sprintf(instructions.sourceStr, "%s", instructions.regStr);

            }
            // destination is in REG field, source is in RM field
            else
            {
                sprintf(instructions.destStr, "%s", instructions.regStr);
                sprintf(instructions.sourceStr, "%s", instructions.rmStr);
            }

            printf("%s %s, %s\n", instructions.instructionStr, instructions.destStr, instructions.sourceStr);
        }
        // add / sub / cmp - immediate to register/memory
        else if ((instructions.byte0 >> 2) == 0b100000)
        {
            uint8 sign = (instructions.byte0 >> 1) & 0b1;
            instructions.width = instructions.byte0 & 0b1;

            // decode mod and r/m
            fread(instructions.bufferPtr, 1, 1, instructions.file);
            instructions.bufferPtr++;
            instructions.mod = (instructions.byte1 >> 6) & 0b11;
            instructions.rm = instructions.byte1 & 0b111;

            // decode instruction type
            // add = 0b000
            if (((instructions.byte1 >> 3) & 0b111) == 0b000)
            {
                sprintf(instructions.instructionStr, "add");
            }
            // sub = 0b101
            else if (((instructions.byte1 >> 3) & 0b111) == 0b101)
            {
                sprintf(instructions.instructionStr, "sub");
            }
            // cmp = 0b111
            else if (((instructions.byte1 >> 3) & 0b111) == 0b111)
            {
                sprintf(instructions.instructionStr, "cmp");
            }
            // unhandled case
            else
            {
                assert(false);
            }

            // decode r/m string
            DecodeRmStr(&instructions);

            // read data. guaranteed to be at least 8-bits.
            int32 data = 0;
            if (sign == 0b0 && instructions.width == 0)
            {
                // read 8-bit unsigned
                fread(instructions.bufferPtr, 1, 1, instructions.file);
                data = (int32)(*(uint8 *)instructions.bufferPtr);
                instructions.bufferPtr++;
            }
            else if (sign == 0b0 && instructions.width == 1)
            {
                // read 16-bit unsigned
                fread(instructions.bufferPtr, 1, 2, instructions.file);
                data = (int32)(*(uint16 *)instructions.bufferPtr);
                instructions.bufferPtr += 2;
            }
            else if (sign == 0b1 && instructions.width == 0)
            {
                // read 8-bit signed
                fread(instructions.bufferPtr, 1, 1, instructions.file);
                data = (int32)(*(int8 *)instructions.bufferPtr);
                instructions.bufferPtr++;
            }
            else if (sign == 0b1 && instructions.width == 1)
            {
                // read 8-bits and sign-extend to 16-bits
                fread(instructions.bufferPtr, 1, 1, instructions.file);
                data = (int32)(*(int8 *)instructions.bufferPtr);
                instructions.bufferPtr++;
            }

            // prepend width hint when immediate is being assigned to memory
            bool prependWidth = ((instructions.mod == 0b0)
                                 || (instructions.mod == 0b1)
                                 || (instructions.mod == 0b10));

            if (prependWidth)
            {
                sprintf(instructions.destStr,
                        "%s %s",
                        ((instructions.width == 0) ? "byte" : "word"),
                        instructions.rmStr);
            }
            else
            {
                sprintf(instructions.destStr, "%s", instructions.rmStr);
            }
            sprintf(instructions.sourceStr, "%i", data);

            printf("%s %s, %s\n", instructions.instructionStr, instructions.destStr, instructions.sourceStr);
        }
        // add / sub / cmp - immediate to/from/with accumulator
        else if (((instructions.byte0 >> 1) == 0b0000010)
            || ((instructions.byte0 >> 1) == 0b0010110)
            || ((instructions.byte0 >> 1) == 0b0011110))
        {
            instructions.width = instructions.byte0 & 0b1;

            // decode instruction type
            // add = 0b000
            if (((instructions.byte0 >> 3) & 0b111) == 0b000)
            {
                sprintf(instructions.instructionStr, "add");
            }
            // sub = 0b101
            else if (((instructions.byte0 >> 3) & 0b111) == 0b101)
            {
                sprintf(instructions.instructionStr, "sub");
            }
            // cmp = 0b111
            else if (((instructions.byte0 >> 3) & 0b111) == 0b111)
            {
                sprintf(instructions.instructionStr, "cmp");
            }
            // unhandled case
            else
            {
                assert(false);
            }

            // read data
            uint16 data = 0;
            if (instructions.width == 0b0)
            {
                fread(instructions.bufferPtr, 1, 1, instructions.file);
                data = (uint16)(*(uint8 *)instructions.bufferPtr);
                instructions.bufferPtr++;
            }
            else if (instructions.width == 0b1)
            {
                fread(instructions.bufferPtr, 1, 2, instructions.file);
                data = (uint16)(*(uint16 *)instructions.bufferPtr);
                instructions.bufferPtr += 2;
            }

            sprintf(instructions.destStr, (instructions.width == 0b0) ? "al" : "ax");
            sprintf(instructions.sourceStr, "[%i]", data);

            printf("%s %s, %s\n", instructions.instructionStr, instructions.destStr, instructions.sourceStr);
        }
        // control transfer instructions
        else if ((instructions.byte0 == 0b01110101)     // jnz / jne
                 || (instructions.byte0 == 0b01110100)  // je
                 || (instructions.byte0 == 0b01111100)  // jl
                 || (instructions.byte0 == 0b01111110)  // jle
                 || (instructions.byte0 == 0b01110010)  // jb
                 || (instructions.byte0 == 0b01110110)  // jbe
                 || (instructions.byte0 == 0b01111010)  // jp
                 || (instructions.byte0 == 0b01110000)  // jo
                 || (instructions.byte0 == 0b01111000)  // js
                 || (instructions.byte0 == 0b01111101)  // jnl
                 || (instructions.byte0 == 0b01111111)  // jg
                 || (instructions.byte0 == 0b01110011)  // jnb
                 || (instructions.byte0 == 0b01110111)  // ja
                 || (instructions.byte0 == 0b01111011)  // jnp
                 || (instructions.byte0 == 0b01110001)  // jno
                 || (instructions.byte0 == 0b01111001)  // jns
                 || (instructions.byte0 == 0b11100010)  // loop
                 || (instructions.byte0 == 0b11100001)  // loopz
                 || (instructions.byte0 == 0b11100000)  // loopnz
                 || (instructions.byte0 == 0b11100011)) // jcxz
        {
            switch(instructions.byte0)
            {
                // jnz / jne
                case 0b01110101:
                    sprintf(instructions.instructionStr, "jnz");
                    break;

                // je
                case 0b01110100:
                    sprintf(instructions.instructionStr, "je");
                    break;

                // jl
                case 0b01111100:
                    sprintf(instructions.instructionStr, "jl");
                    break;

                // jle
                case 0b01111110:
                    sprintf(instructions.instructionStr, "jle");
                    break;

                // jb
                case 0b01110010:
                    sprintf(instructions.instructionStr, "jb");
                    break;

                // jbe
                case 0b01110110:
                    sprintf(instructions.instructionStr, "jbe");
                    break;

                // jp
                case 0b01111010:
                    sprintf(instructions.instructionStr, "jp");
                    break;

                // jo
                case 0b01110000:
                    sprintf(instructions.instructionStr, "jo");
                    break;

                // js
                case 0b01111000:
                    sprintf(instructions.instructionStr, "js");
                    break;

                // jnl
                case 0b01111101:
                    sprintf(instructions.instructionStr, "jnl");
                    break;

                // jg
                case 0b01111111:
                    sprintf(instructions.instructionStr, "jg");
                    break;

                // jnb
                case 0b01110011:
                    sprintf(instructions.instructionStr, "jnb");
                    break;

                // ja
                case 0b01110111:
                    sprintf(instructions.instructionStr, "ja");
                    break;

                // jnp
                case 0b01111011:
                    sprintf(instructions.instructionStr, "jnp");
                    break;

                // jno
                case 0b01110001:
                    sprintf(instructions.instructionStr, "jno");
                    break;

                // jns
                case 0b01111001:
                    sprintf(instructions.instructionStr, "jns");
                    break;

                // loop
                case 0b11100010:
                    sprintf(instructions.instructionStr, "LOOP");
                    break;

                // loopz
                case 0b11100001:
                    sprintf(instructions.instructionStr, "LOOPZ");
                    break;

                // loopnz
                case 0b11100000:
                    sprintf(instructions.instructionStr, "LOOPNZ");
                    break;

                // jcxz
                case 0b11100011:
                    sprintf(instructions.instructionStr, "JCXZ");
                    break;
                default:
                    // unhandled instruction
                    assert(false);
            }

            // read 8-bit signed offset
            bytesRead = fread(instructions.bufferPtr, 1, 1, instructions.file);
            int8 offset = *(int8 *)instructions.bufferPtr;
            instructions.bufferPtr++;

            printf("%s %i\n", instructions.instructionStr, offset);
        }
        else
        {
            // Note (Aaron): Unsupported instruction
        }

#if SIM8086_SLOW
        ClearInstructionData(&instructions);
#endif

        // read the next initial instruction byte
        instructions.bufferPtr = instructions.buffer;
        bytesRead = fread(instructions.bufferPtr, 1, 1, instructions.file);
        instructions.bufferPtr++;
        instructions.instructionCount++;
    }

    fclose(instructions.file);

    return 0;
}
