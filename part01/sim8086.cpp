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

    uint8 instructionBuffer[6] = {};
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

    // read initial instruction byte for parsing
    bytesRead = fread(bufferPtr, 1, 1, file);
    instructionCounter++;

    // main loop
    while(bytesRead)
    {
        // parse instruction & produce disassembly
        uint8 byte0 = instructionBuffer[0];

        const char *instructionStr = "?";
        const char *destStr = "?";
        const char *sourceStr = "?";

        // mov - register/memory to/from register (0b100010)
        if ((byte0 >> 2) == 34)
        {
            instructionStr = "mov";

            // parse initial instruction byte
            uint8 direction = (byte0 >> 1) & (0b00000001);
            uint8 width = byte0 & 0b00000001;

            // read second instruction byte and parse it
            bufferPtr = instructionBuffer + 1;
            fread(bufferPtr, 1, 1, file);
            uint8 byte1 = instructionBuffer[1];

            uint8 mod = (byte1 >> 6);
            uint8 reg = (byte1 >> 3) & (0b00000111);
            uint8 rm = byte1 & 0b00000111;

            // determine how many bits of displacement to read and read it

            // memory mode, no displacement
            if(mod == 0b0)
            {
            }
            // 8-bit displacement
            else if (mod == 0b01)
            {
            }
            // 16-bit displacement
            else if (mod == 0b10)
            {
            }
            // register mode, no displacement
            else if (mod == 0b11)
            {
                if (width == 0b0)
                {
                    destStr = (direction == 1) ? RegisterEncodingsW0[reg] : RegisterEncodingsW0[rm];
                    sourceStr = (direction == 1) ? RegisterEncodingsW0[rm] : RegisterEncodingsW0[reg];
                }
                else if (width == 0b1)
                {
                    destStr = (direction == 1) ? RegisterEncodingsW1[reg] : RegisterEncodingsW1[rm];
                    sourceStr = (direction == 1) ? RegisterEncodingsW1[rm] : RegisterEncodingsW1[reg];
                }
                else
                {
                    Assert(false);
                }
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

        printf("%s %s,%s\n", instructionStr, destStr, sourceStr);

        // read next instructions
        bufferPtr = instructionBuffer;
        bytesRead = fread(bufferPtr, 1, 1, file);
        instructionCounter++;
    }

    fclose(file);

    return 0;
}
