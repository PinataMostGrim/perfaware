#include "imgui.h"
#include <stdio.h>

#include "base_inc.h"
#include "sim8086_platform.h"
#include "sim8086.h"
#include "sim8086_mnemonics.h"
#include "sim8086_gui.h"

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


// Note (Aaron): Push output string into a memory arena that behaves like a circular buffer
global_function void PushOutputToArena(memory_arena *arena, Str8List *outputList, Str8 output)
{
    // //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Note (Aaron): I think this is the algorithm:

    // if (first < last string) && (new string size + node) < (free size in arena)
    //  - We are still first time through the arena and don't need to free first strings
    //  push the new string into the arena and return

    // if (size of new string + node) > (free size in arena)
    //  - we need to reset the arena and potentially start freeing first strings
    //  if (first > last string)
    //    - free first strings until (first string < last string)
    //  reset the arena

    // - now we need to free up first strings until there is room between the arena pos pointer and the first string
    // while ((space between arena->PosPtr and first str) < (new string size + node))
    //  - free first strings

    // push new string into the arena and return

    // free string involves:
    //  - subtract string size from list's total size
    //  - decrement string node count by one
    //  - update list so first = first->next
    // //////////////////////////////////////////////////////////////////////////////////////////////////////////////

    memory_index freeSizeBytes = arena->Size - arena->Used;
    // Note (Aaron): Size of incoming string + null terminating character + the Str8Node that will get created for it
    memory_index stringSizeBytes = output.Length + 1 + sizeof(Str8Node);

    U8 *first = outputList->First ? outputList->First->String.Str : 0;
    U8 *last = outputList->Last ? outputList->Last->String.Str : 0;

    Assert(stringSizeBytes <= arena->Size && "Output arena not large enough to hold output string");

    // first time through the arena, no strings need to be released
    if (first <= last && stringSizeBytes < freeSizeBytes)
    {
        Str8 string = ArenaPushStr8Copy(arena, output, TRUE);
        Str8ListPush(arena, outputList, string);

        return;
    }

    // reached the end of the arena and need to reset it
    if (stringSizeBytes > freeSizeBytes)
    {
        // release "first" strings until they are behind the arena write pointer
        while (first > arena->PositionPtr)
        {
            outputList->TotalSize -= outputList->First->String.Length;
            outputList->NodeCount--;

            // Note (Aaron): outputList->NodeCount should never reach zero as the arena should be large enough to hold many lines of output.
            // Likewise, we should never free so many strings that (outputList->First == outputList->Last).
            Assert(outputList->NodeCount > 0 && "NodeCount should never reach 0");
            Assert(outputList->First->Next && "outputList->First->Next should always have a valid target");
            Assert(outputList->First->String.Str != outputList->Last->String.Str && "outputList->First should never equal outputList->Last");

            outputList->First = outputList->First->Next;
            first = outputList->First->String.Str;
        }

        // reset the arena
        ArenaClear(arena);
    }

    // Release "first" strings until we have enough room to push the new string into the arena
    freeSizeBytes = first - arena->PositionPtr;
    while(freeSizeBytes < stringSizeBytes)
    {
        // free first string
        outputList->TotalSize -= outputList->First->String.Length;
        outputList->NodeCount--;

        // Note (Aaron): outputList->NodeCount should never reach zero as the arena should be large enough to hold many lines of output.
        // Likewise, we should never free so many strings that (outputList->First == outputList->Last).
        Assert(outputList->NodeCount > 0 && "NodeCount should never reach 0");
        Assert(outputList->First->Next && "outputList->First->Next should always have a valid target");
        Assert(outputList->First->String.Str != outputList->Last->String.Str && "outputList->First should never equal outputList->Last");

        outputList->First = outputList->First->Next;
        first = outputList->First->String.Str;
        freeSizeBytes = first - arena->PositionPtr;
    }

    Str8 string = ArenaPushStr8Copy(arena, output, TRUE);
    Str8ListPush(arena, outputList, string);
}


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
        ArenaClearZero(&memory->Instructions.Arena);
        ArenaClearZero(&memory->InstructionStrings.Arena);

        while (processor->IP < processor->ProgramSize)
        {
            instruction nextInstruction = DecodeNextInstruction(processor);
            instruction *nextInstructionPtr = ArenaPushStruct(&memory->Instructions.Arena, instruction);
            nextInstruction.InstructionMnemonic = GetInstructionMnemonic(&nextInstruction, &memory->InstructionStrings.Arena);
            nextInstruction.BitsMnemonic = GetInstructionBitsMnemonic(&nextInstruction, &memory->InstructionStrings.Arena);
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
            Str8 output = ExecuteInstruction(processor, &inst, &memory->Scratch.Arena);
            PushOutputToArena(&memory->Output.Arena, &applicationState->OutputList, output);
            ArenaClear(&memory->Scratch.Arena);

            // TODO (Aaron): How to better handle programs that do not halt?
            safetyCounter++;
            if (safetyCounter > HALT_GUARD_COUNT)
            {
                applicationState->Diagnostics_ExecutionStalled = true;
                break;
            }
        }
    }
    // TODO (Aaron): Change this to shift + F5?
    // else if(ImGui::IsKeyPressed(ImGuiKey_F5) && ImGui::IsKeyDown(ImGuiKey_ModShift))
    else if(ImGui::IsKeyPressed(ImGuiKey_F8))
    {
        // reset program
        ResetProcessorExecution(processor);
        applicationState->Diagnostics_ExecutionStalled = false;
        applicationState->OutputList = {0};
        ArenaClearZero(&memory->Output.Arena);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_F10))
    {
        // execute single instruction
        if (!HasProcessorFinishedExecution(processor))
        {
            instruction inst = DecodeNextInstruction(processor);
            Str8 output = ExecuteInstruction(processor, &inst, &memory->Scratch.Arena);
            PushOutputToArena(&memory->Output.Arena, &applicationState->OutputList, output);
            ArenaClear(&memory->Scratch.Arena);
        }

        applicationState->Diagnostics_ExecutionStalled = false;
    }

    DrawGui(applicationState, memory, processor);
}
