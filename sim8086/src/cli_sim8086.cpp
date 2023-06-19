/* TODO (Aaron):
*/

#include "base.h"
#include "memory_arena.h"

#include "memory_arena.c"
#include "sim8086.cpp"
#include "sim8086_mnemonics.cpp"


void PrintFlags(processor_8086 *processor, bool force = false)
{
    if (processor->Flags == 0 && !force)
    {
        return;
    }

    printf(" flags:->");

    if (processor->Flags & RegisterFlag_CF) { printf("C"); }
    if (processor->Flags & RegisterFlag_PF) { printf("P"); }
    if (processor->Flags & RegisterFlag_AF) { printf("A"); }
    if (processor->Flags & RegisterFlag_ZF) { printf("Z"); }
    if (processor->Flags & RegisterFlag_SF) { printf("S"); }
    if (processor->Flags & RegisterFlag_OF) { printf("O"); }
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
                if (operand.Memory.Flags & Memory_HasDirectAddress)
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
                    S8 offset = (S8)(operand.Immediate.Value & 0xff);

                    // Note (Aaron): Offset the value to accommodate a NASM syntax peculiarity.
                    // NASM expects an offset value from the start of the instruction rather than
                    // the end (which is how the instructions are encoded).
                    offset += instruction->Bits.ByteCount;

                    printf(offset >= 0 ? "$+%i" : "$%i", offset);
                    break;
                }

                // TODO (Aaron): Test this more
                bool isSigned = operand.Immediate.Flags & Immediate_IsSigned;
                printf("%i", isSigned
                       ? (S16) operand.Immediate.Value
                       : (U16) operand.Immediate.Value);

                break;
            }

            default:
            {
                printf("?");
            }
        }
    }
}

static void PrintClocks(processor_8086 *processor, instruction *instruction)
{
    if (instruction->EAClockCount > 0)
    {
        printf(" Clocks: +%i (%i + %iea) = %i",
               (instruction->ClockCount + instruction->EAClockCount),
               instruction->ClockCount,
               instruction->EAClockCount,
               processor->TotalClockCount);
        return;
    }

    printf(" Clocks: +%i = %i", instruction->ClockCount, processor->TotalClockCount);
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
        U16 value = GetRegisterValue(processor, toDisplay[i]);

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

void PrintUsage()
{
    printf("usage: sim8086 [--exec --show-clocks --dump --help] filename\n\n");
    printf("disassembles 8086/88 assembly and optionally simulates it. note: supports \na limited number of instructions.\n\n");

    printf("positional arguments:\n");
    printf("  filename\t\tassembly file to load\n");
    printf("\n");

    printf("options:\n");
    printf("  --exec, -e\t\tsimulate execution of assembly\n");
    printf("  --show-clocks, -c\tshow clock count estimate for each instruction\n");
    printf("  --dump, -d\t\tdump simulation memory to file after execution (%s)\n", MemoryDumpFilename);
    printf("  --help, -h\t\tshow this message\n");
}


int main(int argc, char const *argv[])
{
    static_assert_8086(ArrayCount(OperationMnemonics) == Op_count,
              "OperationMnemonics does not accommodate all operation_types");

    static_assert_8086(ArrayCount(RegisterLookup) == Reg_mem_id_count,
                  "RegisterLookup does not contain definitions for all register IDs");

    if (argc < 2 ||  argc > 5)
    {
        PrintUsage();
        exit(1);
    }

    // TODO (Aaron): Turn this into a struct
    // TODO (Aaron): Extract this into a separate method
    // process command line arguments
    bool simulateInstructions = false;
    bool dumpMemoryToFile = false;
    bool showClocks = false;
    bool stopOnReturn = false;
    const char *filename = "";

    for (int i = 1; i < argc; ++i)
    {
        if ((strncmp("--help", argv[i], 6) == 0)
            || (strncmp("-h", argv[i], 2) == 0))
        {
            PrintUsage();
            exit(0);
        }

        if ((strncmp("--exec", argv[i], 6) == 0)
            || (strncmp("-e", argv[i], 2) == 0))
        {
            simulateInstructions = true;
            continue;
        }

        if ((strncmp("--show-clocks", argv[i], 13) == 0)
            || (strncmp("-c", argv[i], 2) == 0))
        {
            showClocks = true;
            continue;
        }

        if ((strncmp("--dump", argv[i], 6) == 0)
            || (strncmp("-d", argv[i], 2) == 0))
        {
            dumpMemoryToFile = true;
            continue;
        }

        if ((strncmp("--stop-on-ret", argv[i], 13) == 0)
            || (strncmp("-r", argv[i], 2) == 0))
        {
            stopOnReturn = true;
            continue;
        }

        filename = argv[i];
    }

    // initialize processor
    processor_8086 processor = {};
    processor.Memory = (U8 *)calloc(processor.MemorySize, sizeof(U8));

    if (!processor.Memory)
    {
        printf("ERROR: Unable to allocate main memory for 8086\n");
        exit(1);
    }

    FILE *file = {};
    file = fopen(filename, "rb");

    if(!file)
    {
        printf("ERROR: Unable to open '%s'\n", filename);
        exit(1);
    }

    processor.ProgramSize = (U16)fread(processor.Memory, 1, processor.MemorySize, file);

    // error handling for file read
    if (ferror(file))
    {
        printf("ERROR: Encountered error while reading file '%s'", filename);
        exit(1);
    }

    if (!feof(file))
    {
        printf("ERROR: Program size exceeds processor memory; unable to load\n\n");
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

        if (showClocks || simulateInstructions)
        {
            printf(" ;");
        }

        if (showClocks)
        {
            PrintClocks(&processor, &instruction);
        }

        if (showClocks && simulateInstructions)
        {
            printf(" |");
        }

        if (simulateInstructions)
        {
            ExecuteInstruction(&processor, &instruction);
        }

        printf("\n");

        // TODO (Aaron): Test this. Will have to write an assembly specifically to do this as the listings provided
        // contain instructions I haven't supported yet.
        if (instruction.OpType == Op_ret && stopOnReturn)
        {
            break;
        }
    }

    if (simulateInstructions)
    {
        printf("\n");
        PrintRegisters(&processor);
        printf("\n");

        if (dumpMemoryToFile)
        {
            DumpMemoryToFile(&processor, MemoryDumpFilename);
        }
    }

    return 0;
}
