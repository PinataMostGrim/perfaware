#ifndef SIM8086_PLATFORM_H
#define SIM8086_PLATFORM_H

#include "imgui.h"
#include <stdbool.h>

#include "base.h"
#include "base_types.h"
#include "base_memory.h"
#include "base_string.h"
#include "sim8086.h"


// SIM8086_SLOW:
//     0 - No slow code allowed!
//     1 - Slow code welcome

// SIM8086_DIAGNOSTICS:
//     0 - Disabled
//     1 - Enabled


typedef struct memory_arena_def memory_arena_def;
struct memory_arena_def
{
    memory_index Size;
    memory_arena Arena;
    const char *Label;
};


typedef struct application_memory application_memory;
struct application_memory
{
    void *BackingStore;
    U64 TotalSize;

    union
    {
        memory_arena_def Defs[5] = {
            { Megabytes(2), {0}, "Permanent"},
            { Megabytes(1), {0}, "Scratch"},
            { Megabytes(1), {0}, "Instructions"},
            { Megabytes(1), {0}, "InstructionStrings"},
            { Megabytes(64), {0}, "Output"},
        };
        struct
        {
            memory_arena_def Permanent;
            memory_arena_def Scratch;
            memory_arena_def Instructions;
            memory_arena_def InstructionStrings;
            memory_arena_def Output;
        };
    };


    B32 IsInitialized;
};


typedef struct
{
    // SIM8086
    B32 ProgramLoaded;
    B32 LoadFailure;
    U32 LoadedProgramInstructionCount;
    U32 LoadedProgramCycleCount;
    Str8List OutputList;

    // GUI
    ImGuiIO *IO;
    ImVec4 ClearColor;
    U32 Disassembly_SelectedLine;
    U32 Memory_StartAddress;

    // Diagnostics
    bool Diagnostics_ShowWindow;
    bool Diagnostics_ExecutionStalled;

} application_state;


C_LINKAGE_BEGIN
#define SET_IMGUI_CONTEXT(name) void name(ImGuiContext *context)
typedef SET_IMGUI_CONTEXT(set_imgui_context);

#define UPDATE_AND_RENDER(name) void name(application_state *applicationState, application_memory *memory, processor_8086 *processor)
typedef UPDATE_AND_RENDER(update_and_render);

C_LINKAGE_END

#endif // SIM8086_PLATFORM_H
