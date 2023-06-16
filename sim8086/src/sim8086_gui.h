#ifndef SIM8086_GUI_H
#define SIM8086_GUI_H

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"

#include <stdbool.h>

#include "base.h"
#include "sim8086.h"

C_LINKAGE_BEGIN
void SetImguiContext(ImGuiContext *context);
void DrawGui(ImGuiIO *io, bool *show_demo_window, bool *show_another_window, ImVec4 *clear_color, processor_8086 *processor);
C_LINKAGE_END


#endif // SIM8086_GUI_H