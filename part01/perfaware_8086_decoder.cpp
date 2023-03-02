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

global_variable const char *RegisterEncodingsW0[]
{
    "al",
    "cl",
    "dl",
    "bl",
    "ah",
    "ch",
    "dh",
    "bh",
};

global_variable const char *RegisterEncodingsW1[]
{
    "ax",
    "cx",
    "dx",
    "bx",
    "sp",
    "bp",
    "si",
    "di",
};

internal void decode_instruction(uint8 byte1, uint8 byte2)
{
    const char *instructionStr;
    const char *destStr;
    const char *sourceStr;

    uint8 operation = byte1 >> 2;
    uint8 direction = (byte1 >> 1) & (0b00000001);
    uint8 w = byte1 & 0b00000001;

    uint8 mod = (byte2 >> 6);
    uint8 reg = (byte2 >> 3) & (0b00000111);
    uint8 rm = byte2 & 0b00000111;

    if (operation == 34)
    {
        instructionStr = "mov";
        if(mod == 3)
        {
            if (w == 1)
            {
                destStr = (direction == 1) ? RegisterEncodingsW1[reg] : RegisterEncodingsW1[rm];
                sourceStr = (direction == 1) ? RegisterEncodingsW1[rm] : RegisterEncodingsW1[reg];
            }
            else
            {
                destStr = (direction == 1) ? RegisterEncodingsW0[reg] : RegisterEncodingsW0[rm];
                sourceStr = (direction == 1) ? RegisterEncodingsW0[rm] : RegisterEncodingsW0[reg];
            }
        }
        else
        {
            destStr = "?";
            sourceStr = "?";
        }
    }
    else
    {
        instructionStr = "?";
        destStr = "?";
        sourceStr = "?";
    }

    printf("%s %s,%s\n", instructionStr, destStr, sourceStr);
}


int main(int argc, char const *argv[])
{
    // TODO (Aaron): accept command line argument for file

    // char filename[] = "listing_0037_single_register_mov";
    char filename[] = "listing_0038_many_register_mov";

    FILE *file;
    errno_t error;
    size_t bytesRead;

    uint8 instructionBuffer[2] = {};
    uint8 *bufferPtr = instructionBuffer;
    uint32 instructionCounter = 0;

    error = fopen_s(&file, filename, "rb");
    if(!file)
    {
        printf("Unable to open '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    printf("; %s:\n", filename);
    printf("bits 16\n");

    // read file in 16 bit chunks
    // TODO (Aaron): Eventually, the length read will have to depend on the instruction
    bytesRead = fread(bufferPtr, 1, 2, file);
    instructionCounter++;
    while(bytesRead)
    {
        Assert(bytesRead == 2);

        // parse instructions & produce disassembly
        decode_instruction(instructionBuffer[0], instructionBuffer[1]);

        // read next instructions
        bytesRead = fread(bufferPtr, 1, 2, file);
        instructionCounter++;
    }

    fclose(file);

    return 0;
}
