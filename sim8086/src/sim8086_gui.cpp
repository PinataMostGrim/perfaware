/*TODO (Aaron):
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
#include "sim8086_mnemonics.h"


global_function void ShowMainMenuBar(application_state *applicationState)
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

        if (ImGui::BeginMenu("View"))
        {

#if SIM8086_DIAGNOSTICS
            if (ImGui::MenuItem("Show Diagnostics",
                                NULL,
                                applicationState->Diagnostics_ShowWindow))
            {
                applicationState->Diagnostics_ShowWindow = !applicationState->Diagnostics_ShowWindow;
            }
#endif

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}


global_function void ShowAssemblyWindow(application_state *applicationState, memory_arena *instructionArena, memory_arena *frameArena)
{
    size_t instructionCount = instructionArena->Used / sizeof(instruction);
    instruction *instructions = (instruction *)instructionArena->BasePtr;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Assembly", NULL, windowFlags);

    for (U32 i = 0; i < instructionCount; i++)
    {
        instruction currentInstruction = instructions[i];

        // TODO (Aaron): Consider how to more cleanly pick buffer sizes here
        char buffer[64];
        sprintf(buffer, "%i", i + 1);
        if (ImGui::Selectable(buffer, applicationState->Assembly_SelectedLine == i)) { applicationState->Assembly_SelectedLine = i; }

        sprintf(buffer, "0x%.8x", currentInstruction.Address);
        ImGui::SameLine(60);
        ImGui::Text("%s", buffer);

        char *assemblyPtr = GetInstructionMnemonic(&currentInstruction, frameArena);
        ImGui::SameLine(200);
        ImGui::Text("%s", assemblyPtr);

        if (ImGui::IsItemHovered())
        {
            char *bitsString = GetInstructionBitsMnemonic(instructions[i], frameArena);
            ImGui::SetTooltip("%s", bitsString);
        }
    }

    ImGui::End();
}


global_function void ShowRegistersWindow(application_state *applicationState, processor_8086 *processor)
{
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Registers", NULL, windowFlags);

    ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

    // instruction pointer
    if (ImGui::BeginTable("instruction_pointer", 1, tableFlags))
    {
        char buffer[56];
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        sprintf(buffer, "Instruction pointer: 0x%.32x", processor->IP);
        ImGui::TextUnformatted(buffer);
        ImGui::EndTable();
    }

    // al / ah / cl / ch / dl / dh / bl / bh
    {
        char buffer[32];
        register_id registers[] = {
            Reg_al, Reg_ah,
            Reg_cl, Reg_ch,
            Reg_dl, Reg_dh,
            Reg_bl, Reg_bh };

        Assert(ArrayCount(registers) % 2 == 0);

        if (ImGui::BeginTable("registers_low/high", 2, tableFlags))
        {
            for (int i = 0; i < ArrayCount(registers); i = i+2)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                sprintf(buffer, "%s: 0x%.8x", GetRegisterMnemonic(registers[i]), GetRegisterValue(processor, registers[i]));
                ImGui::TextUnformatted(buffer);

                ImGui::TableNextColumn();
                sprintf(buffer, "%s: 0x%.8x", GetRegisterMnemonic(registers[i+1]), GetRegisterValue(processor, registers[i+1]));
                ImGui::TextUnformatted(buffer);
            }

            ImGui::EndTable();
        }
    }

    // ax / cx / dx / bx
    {
        char buffer[32];
        register_id registers[] ={ Reg_ax, Reg_cx, Reg_dx, Reg_bx };

        if (ImGui::BeginTable("registers_full", 1, tableFlags))
        {
            for (int i = 0; i < ArrayCount(registers); i++)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                sprintf(buffer, "%s: 0x%.16x", GetRegisterMnemonic(registers[i]), GetRegisterValue(processor, registers[i]));
                ImGui::TextUnformatted(buffer);
            }

            ImGui::EndTable();
        }
    }

    // sp / bp / si / di
    if (ImGui::BeginTable("registers_1", 1, tableFlags))
    {
        char buffer[32];
        register_id registers[] ={ Reg_sp, Reg_bp, Reg_si, Reg_di };

        for (int i = 0; i < ArrayCount(registers); i++)
        {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            sprintf(buffer, "%s: 0x%.16x", GetRegisterMnemonic(registers[i]), GetRegisterValue(processor, registers[i]));
            ImGui::TextUnformatted(buffer);
        }

        ImGui::EndTable();
    }

    // flags
    if (ImGui::BeginTable("flags", 6, tableFlags))
    {
        char buffer[32];
        register_flags flags[] ={ RegisterFlag_CF, RegisterFlag_PF, RegisterFlag_AF, RegisterFlag_ZF, RegisterFlag_SF, RegisterFlag_OF };

        ImGui::TableNextRow();
        for (int i = 0; i < ArrayCount(flags); i++)
        {
            ImGui::TableNextColumn();
            sprintf(buffer, "%s: 0x%.1u", GetRegisterFlagMnemonic(flags[i]), GetRegisterFlag(processor, flags[i]));
            ImGui::TextUnformatted(buffer);
        }

        ImGui::EndTable();
    }

    ImGui::End();
}


global_function void ShowMemoryWindow(application_state *applicationState, memory_arena *frameArena, processor_8086 *processor)
{
    U8 bytesPerLine = 16;
    U32 bytesDisplayed = Kilobytes(64);
    U32 minimumBytesDisplayed = 512;

    // TODO (Aaron): Consider how to more cleanly pick buffer sizes here
    char buffer[64];
    U32 maxMemoryAddress = processor->MemorySize - 1;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
    ImGui::Begin("Memory", NULL, windowFlags);

    if (ImGui::Button("Previous segment"))
    {
        // protect against underflow
        if (applicationState->Memory_StartAddress < bytesDisplayed)
        {
            applicationState->Memory_StartAddress = 0;
        }
        else
        {
            U32 clampedValue = Clamp(0, applicationState->Memory_StartAddress - bytesDisplayed, maxMemoryAddress - minimumBytesDisplayed);
            applicationState->Memory_StartAddress = clampedValue;
        }

    }

    ImGui::SameLine(132);

    if (ImGui::Button("Next Segment"))
    {
        U32 clampedValue = Clamp(0 + minimumBytesDisplayed, applicationState->Memory_StartAddress + bytesDisplayed, maxMemoryAddress);
        applicationState->Memory_StartAddress = clampedValue;
    }

    U64 startAddress = Clamp(0, applicationState->Memory_StartAddress, maxMemoryAddress - minimumBytesDisplayed);
    U64 endAddress = Clamp(0 + minimumBytesDisplayed, applicationState->Memory_StartAddress + bytesDisplayed, maxMemoryAddress);

    Assert((startAddress < endAddress) && "startAddress cannot be larger than endAddress");
    Assert((startAddress >= 0) && "Invalid start address");
    Assert((endAddress < processor->MemorySize) && "Invalid end address");

    sprintf(buffer, "Range: 0x%.2llx - 0x%.2llx", startAddress, endAddress);
    ImGui::Text("%s", buffer);
    ImGui::Separator();

    // loop over memory range displayed
    for (U64 i = startAddress; i < endAddress; i += bytesPerLine)
    {
        // display address
        sprintf(buffer, "%.16llx", i);
        ImGui::Text("0x%s:", buffer);

        F32 offset = 150;
        for (int j = 0; j < bytesPerLine; ++j)
        {
            // advance offset to deliniate 32 bits
            if (j % 2 == 0) { offset += 10; }

            ImGui::SameLine(offset);
            U8 value = processor->Memory[i + j];
            ImGui::Text("%.2x", value);
            offset += 20;
        }
    }

    ImGui::End();
}


global_function void ShowDiagnosticsWindow(application_state *applicationState, application_memory *memory)
{
#if SIM8086_DIAGNOSTICS
    if (applicationState->Diagnostics_ShowWindow)
    {
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
        ImGui::Begin("Diagnostics", &applicationState->Diagnostics_ShowWindow, windowFlags);

        ImGui::Text("Performance");
        ImGui::Separator();
        ImGui::Text("Average ms/frame: %.3f", 1000.0f / applicationState->IO->Framerate);
        ImGui::Text("FPS: %.1f ", applicationState->IO->Framerate);

        ImGui::Text("");

        ImGui::Text("Memory");
        ImGui::Separator();
        ImGui::Text("Permanent arena: %.2f / %.f KB (%.2f%%)",
                    (F64)memory->PermanentArena.Used / (F64)Kilobytes(1),
                    (F64)memory->PermanentArena.Size / (F64)Kilobytes(1),
                    (F64)memory->PermanentArena.Used / (F64)memory->PermanentArena.Size);
        ImGui::Text("Per-frame arena: %.2f / %.f KB (%.2f%%)",
                    (F64)memory->FrameArena.Used / (F64)Kilobytes(1),
                    (F64)memory->FrameArena.Size / (F64)Kilobytes(1),
                    (F64)memory->FrameArena.Used / (F64)memory->FrameArena.Size);
        ImGui::Text("Max per-frame arena: %.2f KB", (F64)applicationState->MaxScratchMemoryUsage / (F64)Kilobytes(1));
        ImGui::Text("Instruction arena: %llu / %llu KB (%.2f%%)",
                    memory->InstructionsArena.Used / Kilobytes(1),
                    memory->InstructionsArena.Size / Kilobytes(1),
                    (F64)memory->InstructionsArena.Used / (F64)memory->InstructionsArena.Size);
        ImGui::Text("Total used: %.3f MB (%.2f%%)",
                    (F64)memory->TotalSize / (F64)Megabytes(1),
                    (F64)(memory->PermanentArena.Used + memory->FrameArena.Used + memory->InstructionsArena.Used) / (F64)memory->TotalSize);
        ImGui::Text("");

        ImGui::End();
    }
#endif
}


global_function void DrawGui(application_state *applicationState, application_memory *memory, processor_8086 *processor)
{
    ShowMainMenuBar(applicationState);

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    ShowAssemblyWindow(applicationState, &memory->InstructionsArena, &memory->FrameArena);
    ShowRegistersWindow(applicationState, processor);
    ShowMemoryWindow(applicationState, &memory->FrameArena, processor);

    ShowDiagnosticsWindow(applicationState, memory);

    // ImGui::ShowDemoWindow();
    // ImGui::ShowStackToolWindow();
}
