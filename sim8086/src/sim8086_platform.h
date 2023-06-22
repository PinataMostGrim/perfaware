#ifndef SIM8086_PLATFORM_H
#define SIM8086_PLATFORM_H

#include "imgui.h"
#include <stdbool.h>

#include "base.h"
#include "memory_arena.h"
#include "platform_metrics.h"
#include "sim8086.h"


// SIM8086_SLOW:
//     0 - No slow code allowed!
//     1 - Slow code welcome

// SIM8086_DIAGNOSTICS:
//     0 - Disabled
//     1 - Enabled

#if SIM8086_DIAGNOSTICS
typedef enum
{
  DiagnosticTimings_Frame,
  diagnostic_timings_Count,
} diagnostic_timings;
#endif


typedef struct
{
    void *BackingStore;
    U64 TotalSize;

    memory_arena PermanentArena;
    memory_arena FrameArena;
    memory_arena InstructionsArena;

    B32 IsInitialized;
} application_memory;


typedef struct
{
    // SIM8086
    B32 ProgramLoaded;
    B32 LoadFailure;

    // GUI
    ImGuiIO *IO;
    ImVec4 ClearColor;
    U32 Assembly_SelectedLine;

#if SIM8086_DIAGNOSTICS
    // Diagnostics
    bool Diagnostics_ShowWindow;
    U64 MaxScratchMemoryUsage;

    metric_timing Timings[diagnostic_timings_Count];

#endif
} application_state;


C_LINKAGE_BEGIN
#define SET_IMGUI_CONTEXT(name) void name(ImGuiContext *context)
typedef SET_IMGUI_CONTEXT(set_imgui_context);

#define UPDATE_AND_RENDER(name) void name(application_state *applicationState, application_memory *memory, processor_8086 *processor)
typedef UPDATE_AND_RENDER(update_and_render);

C_LINKAGE_END

#endif // SIM8086_PLATFORM_H
