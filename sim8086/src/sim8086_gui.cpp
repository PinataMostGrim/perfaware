/*TODO (Aaron):
    - Add an FPS counter
*/

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"


#include <stdio.h>
#include <stdbool.h>

#include "base.h"
#include "memory_arena.h"
#include "sim8086_platform.h"
#include "sim8086_gui.h"
#include "sim8086.h"

#include "memory_arena.c"
#include "sim8086.cpp"



C_LINKAGE SET_IMGUI_CONTEXT(SetImguiContext)
{
    ImGui::SetCurrentContext(context);
}


global_function void _DrawMenuBar(gui_state *guiState)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open")) {}
            if (ImGui::MenuItem("Close")) {}
            ImGui::EndMenu();
        }

        // if (ImGui::BeginMenu("Edit"))
        // {
        //     if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
        //     if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
        //     ImGui::Separator();
        //     if (ImGui::MenuItem("Cut", "CTRL+X")) {}
        //     if (ImGui::MenuItem("Copy", "CTRL+C")) {}
        //     if (ImGui::MenuItem("Paste", "CTRL+V")) {}
        //     ImGui::EndMenu();
        // }

        ImGui::EndMainMenuBar();
    }
}


#if 0
global_function void _DrawAssemblyExperiment00(gui_state *guiState)
{
    bool useWorkArea = true;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(useWorkArea ? viewport->WorkPos : viewport->Pos);
    ImGui::SetNextWindowSize(useWorkArea ? viewport->WorkSize : viewport->Size);
    ImGuiWindowFlags backgroundFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Background", NULL, backgroundFlags);

    U32 address = 100000000;
    for (int n = 0; n < 5; n++)
    {
        char lineBuff[32];
        char addressBuf[32];
        char assemblyBuf[32];

        sprintf(lineBuff, "%i", n + 1);
        sprintf(addressBuf, "0x%.8x", address);
        sprintf(assemblyBuf, "mov  eax, [si+28]");

        if (ImGui::Selectable(lineBuff, guiState->SelectedLine == n)) guiState->SelectedLine = n;

        ImGui::SameLine(60);
        ImGui::Text("%s", addressBuf);
        ImGui::SameLine(200);
        ImGui::Text("%s", assemblyBuf);

        address++;
    }

    ImGui::End();
}
#endif


global_function void _DrawAssemblyWindow(gui_state *guiState, memory_arena *instructionArena, memory_arena *frameArena)
{
    size_t instructionCount = instructionArena->Used / sizeof(instruction);
    instruction *instructions = (instruction *)instructionArena->BasePtr;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Assembly", NULL, windowFlags);

    U32 address = 100000000;
    for (U32 i = 0; i < instructionCount; i++)
    {
        instruction currentInstruction = instructions[i];

        // TODO (Aaron): Consider how to more cleanly pick buffer sizes here?
        char lineBuff[32];
        char addressBuf[32];

        sprintf(lineBuff, "%i", i + 1);
        sprintf(addressBuf, "0x%.8x", currentInstruction.Address);
        char *assemblyPtr = GetInstructionMnemonic(&currentInstruction, frameArena);

        if (ImGui::Selectable(lineBuff, guiState->SelectedLine == i)) guiState->SelectedLine = i;

        ImGui::SameLine(60);
        ImGui::Text("%s", addressBuf);
        ImGui::SameLine(200);
        ImGui::Text("%s", assemblyPtr);

        address++;
    }

    ImGui::End();
}


C_LINKAGE DRAW_GUI(DrawGui)
{
    _DrawMenuBar(guiState);
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    _DrawAssemblyWindow(guiState, &memory->InstructionsArena, &memory->FrameArena);

    // ImGui::ShowDemoWindow();
    // ImGui::ShowStackToolWindow();
}
