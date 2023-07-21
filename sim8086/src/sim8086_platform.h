#ifndef SIM8086_PLATFORM_H
#define SIM8086_PLATFORM_H

#include "imgui.h"
#include <stdbool.h>

#include "base.h"
#include "base_types.h"
#include "base_memory.h"
#include "sim8086.h"


// SIM8086_SLOW:
//     0 - No slow code allowed!
//     1 - Slow code welcome

// SIM8086_DIAGNOSTICS:
//     0 - Disabled
//     1 - Enabled


typedef struct application_memory application_memory;
struct application_memory
{
    void *BackingStore;
    U64 TotalSize;

    union
    {
        memory_index ArenaSizes[4];
        struct
        {
            memory_index PermanentArenaSize;
            memory_index ScratchArenaSize;
            memory_index InstructionsArenaSize;
            memory_index InstructionStringsArenaSize;
        };
    };

    union
    {
        memory_arena Arenas[4];
        struct
        {
            memory_arena PermanentArena;
            memory_arena ScratchArena;
            memory_arena InstructionsArena;
            memory_arena InstructionStringsArena;
        };
    };

    static_assert(ArrayCount(Arenas) == ArrayCount(ArenaSizes),
                  "ArenaSizes count must match memory.Arenas count");

    B32 IsInitialized;
};


typedef struct
{
    // SIM8086
    B32 ProgramLoaded;
    B32 LoadFailure;
    U32 LoadedProgramInstructionCount;
    U32 LoadedProgramCycleCount;

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
