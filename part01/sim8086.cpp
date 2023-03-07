#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define internal static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

// TODO (Aaron): Disable after testing
#if 1
#define Assert(Expression) if (!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

struct field_encoding
{
    char *table[8] {};
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
} instruction_buffer;

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

    FILE *file;
    errno_t error;
    size_t bytesRead;

    instruction_buffer instructions = {};

    error = fopen_s(&file, filename, "rb");
    if(!file)
    {
        printf("Unable to open '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    printf("; %s:\n", filename);
    printf("bits 16\n");

    // read initial instruction byte for parsing
    bytesRead = fread(instructions.bufferPtr, 1, 1, file);
    instructions.instructionCount++;

    // main loop
    while(bytesRead)
    {
        // parse initial instruction byte
        char instructionStr[32] = "?";
        char destStr[32] = "?";
        char sourceStr[32] = "?";
        char result[256] = "";

        // mov instruction - register/memory to/from register (0b100010)
        if ((instructions.byte0 >> 2) == 34)
        {
            sprintf_s(instructionStr, "mov");

            // parse initial instruction byte
            uint8 direction = (instructions.byte0 >> 1) & (0b00000001);
            uint8 width = instructions.byte0 & 0b00000001;

            // read second instruction byte and parse it
            instructions.bufferPtr += 1;
            fread(instructions.bufferPtr, 1, 1, file);

            uint8 mod = (instructions.byte1 >> 6);
            uint8 reg = (instructions.byte1 >> 3) & (0b00000111);
            uint8 rm = instructions.byte1 & 0b00000111;

            // memory mode, no displacement
            if(mod == 0b0)
            {
                if(rm == 0b110)
                {
                }
                else
                {
                    // destination is in RM field, source is in REG field
                    if (direction == 0)
                    {
                        sprintf_s(destStr, "[%s]", RegMemEncodings.table[rm]);
                        sprintf_s(sourceStr, "%s", RegisterEncodings[width].table[reg]);
                    }
                    // destination is in REG field, source is in RM field
                    else
                    {
                        sprintf_s(destStr, "%s", RegisterEncodings[width].table[reg]);
                        sprintf_s(sourceStr, "[%s]", RegMemEncodings.table[rm]);
                    }
                }
            }
            // memory mode, 8-bit and 16-bit displacements
            else if ((mod == 0b1) || (mod == 0b10))
            {
                instructions.bufferPtr += 1;
                uint16 displacement = 0;

                if (mod == 0b1)
                {
                    fread(instructions.bufferPtr, 1, 1, file);
                    displacement = (uint32)(*(uint8 *)instructions.bufferPtr);
                }
                else if (mod == 0b10)
                {
                    fread(instructions.bufferPtr, 1, 2, file);
                    displacement = (uint32)(*(uint16 *)instructions.bufferPtr);
                }
                // unhandled case
                else
                {
                    Assert(false);
                }

                // destination is in RM field, source is in REG field
                if (direction == 0)
                {
                    if (displacement == 0)
                    {
                        sprintf_s(destStr, "[%s]", RegMemEncodings.table[rm]);
                    }
                    else
                    {
                        sprintf_s(destStr, "[%s + %i]", RegMemEncodings.table[rm], displacement);
                    }

                    sprintf_s(sourceStr, "%s", RegisterEncodings[width].table[reg]);
                }
                // destination is in REG field, source is in RM field
                else
                {
                    sprintf_s(destStr, "%s", RegisterEncodings[width].table[reg]);

                    if(displacement == 0)
                    {
                        sprintf_s(sourceStr, "[%s]", RegMemEncodings.table[rm]);
                    }
                    else
                    {
                        sprintf_s(sourceStr, "[%s + %i]", RegMemEncodings.table[rm], displacement);
                    }
                }
            }
            // register mode, no displacement
            else if (mod == 0b11)
            {
                sprintf_s(destStr, "%s", (direction == 1) ? RegisterEncodings[width].table[reg] : RegisterEncodings[width].table[rm]);
                sprintf_s(sourceStr, "%s", (direction == 1) ? RegisterEncodings[width].table[rm] : RegisterEncodings[width].table[reg]);
            }
            // unhandled case
            else
            {
                Assert(false);
            }

        }
        // mov instruction - immediate to register (0b1011)
        else if ((instructions.byte0 >> 4) == 0b1011)
        {
            sprintf_s(instructionStr, "mov");

            // parse width and reg
            uint8 width = (instructions.byte0 >>3) & (0b1);
            uint8 reg = instructions.byte0 & (0b111);
            instructions.bufferPtr += 1;

            if (width == 0b0)
            {
                // read 8-bit data
                fread(instructions.bufferPtr, 1, 1, file);
                sprintf_s(sourceStr, "%i", *instructions.bufferPtr);
            }
            else if (width == 0b1)
            {
                // read 16-bit data
                fread(instructions.bufferPtr, 1, 2, file);
                sprintf_s(sourceStr, "%i", *(uint16 *)instructions.bufferPtr);
            }
            // unhandled case
            else
            {
                Assert(false);
            }

            sprintf_s(destStr, "%s", RegisterEncodings[width].table[reg]);
        }
        else
        {
            // Note (Aaron): Unsupported instruction
        }

        sprintf_s(result, "%s %s, %s\n", instructionStr, destStr, sourceStr);
        printf("%s", result);

        instructions.bufferPtr = instructions.buffer;
        // read the next initial instruction byte
        bytesRead = fread(instructions.bufferPtr, 1, 1, file);
        instructions.instructionCount++;
    }

    fclose(file);

    return 0;
}
