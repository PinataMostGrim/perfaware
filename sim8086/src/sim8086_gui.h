#ifndef SIM8086_GUI_H
#define SIM8086_GUI_H

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"

#include <stdbool.h>

#include "base.h"
#include "memory_arena.h"
#include "sim8086_platform.h"
#include "sim8086.h"


typedef struct
{
    ImVec4 ClearColor;
    U32 Assembly_SelectedLine;
} gui_state;


C_LINKAGE_BEGIN
#define SET_IMGUI_CONTEXT(name) void name(ImGuiContext *context)
typedef SET_IMGUI_CONTEXT(set_imgui_context);

#define DRAW_GUI(name) void name(gui_state *guiState, ImGuiIO *io, application_memory *memory, processor_8086 *processor)
typedef DRAW_GUI(draw_gui);
C_LINKAGE_END


#endif // SIM8086_GUI_H
