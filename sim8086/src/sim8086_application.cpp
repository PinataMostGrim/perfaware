#include "imgui.h"
#include <stdio.h>

#include "base_inc.h"
#include "sim8086_platform.h"
#include "sim8086.h"
#include "sim8086_mnemonics.h"
#include "sim8086_gui.h"
#include "platform_metrics.h"

#include "platform_metrics.c"
#include "base_types.c"
#include "base_memory.c"
#include "base_string.c"
#include "sim8086.cpp"
#include "sim8086_mnemonics.cpp"
#include "sim8086_gui.cpp"


#define HALT_GUARD_COUNT 100000


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
        ArenaClearZero(&memory->InstructionsArena);
        ArenaClearZero(&memory->InstructionStringsArena);

        while (processor->IP < processor->ProgramSize)
        {
            instruction nextInstruction = DecodeNextInstruction(processor);
            instruction *nextInstructionPtr = ArenaPushStruct(&memory->InstructionsArena, instruction);
            nextInstruction.InstructionMnemonic = GetInstructionMnemonic(&nextInstruction, &memory->InstructionStringsArena);
            nextInstruction.BitsMnemonic = GetInstructionBitsMnemonic(&nextInstruction, &memory->InstructionStringsArena);
            MemoryCopy(nextInstructionPtr, &nextInstruction, sizeof(instruction));
        }

        applicationState->LoadedProgramInstructionCount = processor->InstructionCount;
        applicationState->LoadedProgramCycleCount = processor->TotalClockCount;

        // Reset processor state to prepare for simulated execution
        ResetProcessorExecution(processor);
        applicationState->ProgramLoaded = TRUE;
    }

    // handle input
    if (ImGui::IsKeyPressed(ImGuiKey_F5))
    {
        // execute entire program
        U32 safetyCounter = 0;
        applicationState->Diagnostics_ExecutionStalled = false;

        while(!HasProcessorFinishedExecution(processor))
        {
            instruction inst = DecodeNextInstruction(processor);
            ExecuteInstruction(processor, &inst);

            // TODO (Aaron): How to better handle programs that do not halt?
            safetyCounter++;
            if (safetyCounter > HALT_GUARD_COUNT)
            {
                applicationState->Diagnostics_ExecutionStalled = true;
                break;
            }
        }
    }
    else if(ImGui::IsKeyPressed(ImGuiKey_F8))
    {
        // reset program
        ResetProcessorExecution(processor);
        applicationState->Diagnostics_ExecutionStalled = false;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_F10))
    {
        // execute single instruction
        if (!HasProcessorFinishedExecution(processor))
        {
            instruction inst = DecodeNextInstruction(processor);
            ExecuteInstruction(processor, &inst);
        }

        applicationState->Diagnostics_ExecutionStalled = false;
    }

    DrawGui(applicationState, memory, processor);
}
