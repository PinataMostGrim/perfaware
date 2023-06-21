#ifndef SIM8086_PLATFORM_H
#define SIM8086_PLATFORM_H

#include "imgui.h"

#include "base.h"
#include "memory_arena.h"
#include "sim8086.h"


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
    // GUI
    ImGuiIO *IO;
    ImVec4 ClearColor;
    U32 Assembly_SelectedLine;
} application_state;


C_LINKAGE_BEGIN
#define SET_IMGUI_CONTEXT(name) void name(ImGuiContext *context)
typedef SET_IMGUI_CONTEXT(set_imgui_context);

#define UPDATE_AND_RENDER(name) void name(application_state *applicationState, application_memory *memory, processor_8086 *processor)
typedef UPDATE_AND_RENDER(update_and_render);

C_LINKAGE_END

#endif // SIM8086_PLATFORM_H
