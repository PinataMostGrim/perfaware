#include "imgui.h"

#include "base.h"
#include "sim8086_platform.h"
#include "sim8086.h"
#include "sim8086_mnemonics.h"
#include "sim8086_gui.h"

#include "memory_arena.c"
#include "sim8086.cpp"
#include "sim8086_mnemonics.cpp"
#include "sim8086_gui.cpp"


C_LINKAGE SET_IMGUI_CONTEXT(SetImGuiContext)
{
    ImGui::SetCurrentContext(context);
}



C_LINKAGE UPDATE_AND_RENDER(UpdateAndRender)
{
    DrawGui(applicationState, memory, processor);
}
