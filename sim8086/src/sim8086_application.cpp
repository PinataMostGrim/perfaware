#include "imgui.h"
#include <stdio.h>

#include "base.h"
#include "memory_arena.h"
#include "sim8086_platform.h"
#include "sim8086.h"
#include "sim8086_mnemonics.h"
#include "sim8086_gui.h"

#include "memory_arena.c"
#include "sim8086.cpp"
#include "sim8086_mnemonics.cpp"
#include "sim8086_gui.cpp"


// global_variable const char * ASSEMBLY_FILE = "..\\listings\\listing_0037_single_register_mov";
// global_variable const char * ASSEMBLY_FILE = "..\\listings\\listing_0039_more_movs";
global_variable const char * ASSEMBLY_FILE = "..\\listings\\listing_0041_add_sub_cmp_jnz";



C_LINKAGE SET_IMGUI_CONTEXT(SetImGuiContext)
{
    ImGui::SetCurrentContext(context);
}



C_LINKAGE UPDATE_AND_RENDER(UpdateAndRender)
{
    if (!applicationState->ProgramLoaded && !applicationState->LoadFailure)
    {
        // load program into the processor
        FILE *file = {};
        file = fopen(ASSEMBLY_FILE, "rb");
        if (!file)
        {
            Assert(FALSE && "Unable to load hard-coded assembly file");
            applicationState->LoadFailure = TRUE;
            return;
        }

        // load program into 8086
        processor->ProgramSize = (U16)fread(processor->Memory, 1, processor->MemorySize, file);

        // error handling for file read
        if (ferror(file))
        {
            Assert(FALSE && "Encountered error while reading file");
            applicationState->LoadFailure = TRUE;
            return;
        }

        if (!feof(file))
        {
            Assert(FALSE && "Program size exceeds processor memory; unable to load");
            applicationState->LoadFailure = TRUE;
            return;
        }

        fclose(file);

        // generate instructions from loaded program
        instruction *instructionBuffer = (instruction *)memory->InstructionsArena.BasePtr;
        while (processor->IP < processor->ProgramSize)
        {
            instruction nextInstruction = DecodeNextInstruction(processor);
            instruction *nextInstructionPtr = ArenaPushStruct(&memory->InstructionsArena, instruction);
            MemoryCopy(nextInstructionPtr, &nextInstruction, sizeof(instruction));
        }

        // reset the instruction pointer
        processor->IP = 0;
        applicationState->LoadedProgramInstructionCount = processor->InstructionCount;
        applicationState->LoadedProgramCycleCount = processor->TotalClockCount;
        applicationState->ProgramLoaded = TRUE;
    }

    DrawGui(applicationState, memory, processor);
}
