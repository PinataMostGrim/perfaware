#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define internal static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;

#pragma warning(disable:4996)   //_CRT_SECURE_NO_WARNINGS
#pragma clang diagnostic ignored "-Wnull-dereference"

#define SIM8086_SLOW 1
// SIM8086_SLOW:
//     0 - No slow code allowed!
//     1 - Slow code welcome


#if SIM8086_SLOW
#define Assert(Expression) if (!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

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

        // set displacement string
        if(instructions->displacement == 0)
        {
            sprintf(instructions->displacementStr, "");
        }
        else if (instructions->displacement < 0)
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
#if SIM8086_SLOW
                instructions.bufferPtr++;
#endif
            }
            else if (instructions.width == 0b1)
            {
                fread(instructions.bufferPtr, 1, 2, instructions.file);
                data = (uint16)(*(uint16 *)instructions.bufferPtr);
#if SIM8086_SLOW
                instructions.bufferPtr += 2;
#endif
            }
            // unhandled case
            else
            {
                Assert(false);
            }

            sprintf(instructions.destStr, "%s", instructions.rmStr);
            sprintf(instructions.sourceStr,
                      "%s %i",
                      ((instructions.width == 0) ? "byte" : "word"),
                      data);
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
                sprintf(instructions.sourceStr, "%i", *instructions.bufferPtr);
#if SIM8086_SLOW
                instructions.bufferPtr++;
#endif
            }
            else if (instructions.width == 0b1)
            {
                // read 16-bit data
                fread(instructions.bufferPtr, 1, 2, instructions.file);
                sprintf(instructions.sourceStr, "%i", *(uint16 *)instructions.bufferPtr);
#if SIM8086_SLOW
                instructions.bufferPtr += 2;
#endif
            }
            // unhandled case
            else
            {
                Assert(false);
            }

            sprintf(instructions.destStr, "%s", RegisterEncodings[instructions.width].table[instructions.reg]);
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
#if SIM8086_SLOW
                instructions.bufferPtr++;
#endif
            }
            else if (instructions.width == 1)
            {
                fread(instructions.bufferPtr, 1, 2, instructions.file);
                instructions.address = (uint16)(*(uint16 *)instructions.bufferPtr);
#if SIM8086_SLOW
                instructions.bufferPtr++;
#endif
            }
            // unhandled case
            else
            {
                Assert(false);
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
                Assert(false);
            }
        }
        else
        {
            // Note (Aaron): Unsupported instruction
        }

        printf("%s %s, %s\n", instructions.instructionStr, instructions.destStr, instructions.sourceStr);

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
