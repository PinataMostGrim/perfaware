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

struct register_encodings
{
    const char *encodings[8] {};
};

global_variable register_encodings RegisterEncodings[2]
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
        // parse instruction & produce disassembly
        const char *instructionStr = "?";
        const char *destStr = "?";
        const char *sourceStr = "?";

        // mov - register/memory to/from register (0b100010)
        if ((instructions.byte0 >> 2) == 34)
        {
            instructionStr = "mov";

            // parse initial instruction byte
            uint8 direction = (instructions.byte0 >> 1) & (0b00000001);
            uint8 width = instructions.byte0 & 0b00000001;

            // read second instruction byte and parse it
            instructions.bufferPtr = instructions.buffer + 1;
            fread(instructions.bufferPtr, 1, 1, file);

            uint8 mod = (instructions.byte1 >> 6);
            uint8 reg = (instructions.byte1 >> 3) & (0b00000111);
            uint8 rm = instructions.byte1 & 0b00000111;

            // determine how many bits of displacement required and read it
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
                destStr = (direction == 1) ? RegisterEncodings[width].encodings[reg] : RegisterEncodings[width].encodings[rm];
                sourceStr = (direction == 1) ? RegisterEncodings[width].encodings[rm] : RegisterEncodings[width].encodings[reg];
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
        instructions.bufferPtr = instructions.buffer;
        bytesRead = fread(instructions.bufferPtr, 1, 1, file);
        instructions.instructionCount++;
    }

    fclose(file);

    return 0;
}