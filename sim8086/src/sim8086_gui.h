#ifndef SIM8086_GUI_H
#define SIM8086_GUI_H

#include "imgui.h"

#include "base.h"
#include "sim8086_platform.h"
#include "sim8086.h"


global_function void DrawGui(gui_state *guiState, ImGuiIO *io, application_memory *memory, processor_8086 *processor);

#endif // SIM8086_GUI_H
