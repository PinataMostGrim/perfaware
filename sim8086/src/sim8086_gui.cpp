/*TODO (Aaron):
*/

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"

#include <stdio.h>
#include <stdbool.h>

#include "base_types.h"
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

        if (ImGui::BeginMenu("Control"))
        {
            // TODO (Aaron): Hmmm... how do I process control from here?
            //  - Seems messy creating sim8086_application.h and making methods for this

            if (ImGui::MenuItem("Run program", "F5", false, false)) {}  // Disabled item
            if (ImGui::MenuItem("Reset program", "F8", false, false)) {}  // Disabled item
            if (ImGui::MenuItem("Step instruction", "F10", false, false)) {}  // Disabled item

            ImGui::EndMenu();
        }

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


global_function void ShowDisassemblyWindow(application_state *applicationState, processor_8086 *processor, memory_arena *instructionArena)
{
    size_t instructionCount = instructionArena->Used / sizeof(instruction);
    instruction *instructions = (instruction *)instructionArena->BasePtr;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Disassembly", NULL, windowFlags);

    for (U32 i = 0; i < instructionCount; i++)
    {
        instruction currentInstruction = instructions[i];

        if (processor->IP == currentInstruction.Address)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        }

        const U8 BUFFER_SIZE = 64;
        char buffer[BUFFER_SIZE];

        snprintf(buffer, BUFFER_SIZE, "%i", i + 1);
        if (ImGui::Selectable(buffer, applicationState->Disassembly_SelectedLine == i)) { applicationState->Disassembly_SelectedLine = i; }

        snprintf(buffer, BUFFER_SIZE, "0x%.8x", currentInstruction.Address);
        ImGui::SameLine(50);
        ImGui::Text("%s", buffer);

        ImGui::SameLine(160);
        ImGui::Text("%s", currentInstruction.InstructionMnemonic.Str);

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", instructions[i].BitsMnemonic.Str);
        }

        if (processor->IP == currentInstruction.Address)
        {
            ImGui::PopStyleColor(1);
        }
    }

    ImGui::End();
}


global_function void ShowRegistersWindow(application_state *applicationState, processor_8086 *processor)
{
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Registers", NULL, windowFlags);

    ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

    const U8 BUFFER_SIZE = 64;
    char buffer[BUFFER_SIZE];

    // instruction pointer
    if (ImGui::BeginTable("instruction_pointer", 1, tableFlags))
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        snprintf(buffer, BUFFER_SIZE, "Instruction pointer: 0x%.32x", processor->IP);
        ImGui::TextUnformatted(buffer);
        ImGui::EndTable();
    }

    // al / ah / cl / ch / dl / dh / bl / bh
    {
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
                snprintf(buffer, BUFFER_SIZE, "%s: 0x%.8x", GetRegisterMnemonic(registers[i]), GetRegisterValue(processor, registers[i]));
                ImGui::TextUnformatted(buffer);

                ImGui::TableNextColumn();
                snprintf(buffer, BUFFER_SIZE, "%s: 0x%.8x", GetRegisterMnemonic(registers[i+1]), GetRegisterValue(processor, registers[i+1]));
                ImGui::TextUnformatted(buffer);
            }

            ImGui::EndTable();
        }
    }

    // ax / cx / dx / bx
    {
        register_id registers[] ={ Reg_ax, Reg_cx, Reg_dx, Reg_bx };

        if (ImGui::BeginTable("registers_full", 1, tableFlags))
        {
            for (int i = 0; i < ArrayCount(registers); i++)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                snprintf(buffer, BUFFER_SIZE, "%s: 0x%.16x", GetRegisterMnemonic(registers[i]), GetRegisterValue(processor, registers[i]));
                ImGui::TextUnformatted(buffer);
            }

            ImGui::EndTable();
        }
    }

    // sp / bp / si / di
    if (ImGui::BeginTable("registers_1", 1, tableFlags))
    {
        register_id registers[] ={ Reg_sp, Reg_bp, Reg_si, Reg_di };

        for (int i = 0; i < ArrayCount(registers); i++)
        {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            snprintf(buffer, BUFFER_SIZE, "%s: 0x%.16x", GetRegisterMnemonic(registers[i]), GetRegisterValue(processor, registers[i]));
            ImGui::TextUnformatted(buffer);
        }

        ImGui::EndTable();
    }

    // flags
    if (ImGui::BeginTable("flags", 6, tableFlags))
    {
        register_flags flags[] ={ RegisterFlag_CF, RegisterFlag_PF, RegisterFlag_AF, RegisterFlag_ZF, RegisterFlag_SF, RegisterFlag_OF };

        ImGui::TableNextRow();
        for (int i = 0; i < ArrayCount(flags); i++)
        {
            ImGui::TableNextColumn();
            snprintf(buffer, BUFFER_SIZE, "%s: 0x%.1u", GetRegisterFlagMnemonic(flags[i]), GetRegisterFlag(processor, flags[i]));
            ImGui::TextUnformatted(buffer);
        }

        ImGui::EndTable();
    }

    ImGui::End();
}


global_function void ShowMemoryWindow(application_state *applicationState, processor_8086 *processor)
{
    U8 bytesPerLine = 16;
    U32 bytesDisplayed = Kilobytes(8);
    U32 minimumBytesDisplayed = 512;

    const U8 BUFFER_SIZE = 64;
    char buffer[BUFFER_SIZE];
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

    snprintf(buffer, BUFFER_SIZE, "Range: 0x%.2llx - 0x%.2llx", startAddress, endAddress);
    ImGui::Text("%s", buffer);
    ImGui::Separator();

    // loop over memory range displayed
    for (U64 i = startAddress; i < endAddress; i += bytesPerLine)
    {
        // display address
        snprintf(buffer, BUFFER_SIZE, "%.16llx", i);
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


inline
global_function void _ImGuiTextLabelUsedTotalPercentage(char *label, char *units, F64 used, F64 total)
{
    ImGui::Text("%s: %.2f / %.f %s (%.2f%%)",
                label,
                used,
                total,
                units,
                used / total * 100);
}


global_function void ShowDiagnosticsWindow(application_state *applicationState, application_memory *memory, processor_8086 *processor)
{
    if (applicationState->Diagnostics_ShowWindow)
    {
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
        ImGui::Begin("Diagnostics", &applicationState->Diagnostics_ShowWindow, windowFlags);


        ImGui::Text("8086");
        ImGui::Separator();

        ImGui::Text("Memory capacity: %llu KB", processor->MemorySize / Kilobytes(1));
        ImGui::Text("Loaded program size: %u bytes", processor->ProgramSize);
        ImGui::Text("Instruction count: %u", applicationState->LoadedProgramInstructionCount);
        ImGui::Text("Estimated cycle count: %u", applicationState->LoadedProgramCycleCount);
        ImGui::Text("");

        ImGui::Text("Instructions executed: %u", processor->InstructionCount);

        if(applicationState->Diagnostics_ExecutionStalled)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::Text("Execution timed out!");
            ImGui::PopStyleColor(1);
        }
        ImGui::Text("");

        ImGui::Text("Memory");
        ImGui::Separator();

        for (int i = 0; i < ArrayCount(memory->Defs); ++i)
        {
            _ImGuiTextLabelUsedTotalPercentage(
                (char *)memory->Defs[i].Label,
                (char *)"KB",
                (F64)memory->Defs[i].Arena.Used / Kilobytes(1),
                (F64)memory->Defs[i].Arena.Size / Kilobytes(1));
        }

        U64 totalUsed = 0;
        for (int i = 0; i < ArrayCount(memory->Defs); ++i)
        {
            totalUsed += memory->Defs[i].Arena.Used;
        }

        _ImGuiTextLabelUsedTotalPercentage(
            (char *)"Total",
            (char *)"MB",
            (F64)totalUsed / Megabytes(1),
            (F64)memory->TotalSize / Megabytes(1));

        ImGui::Text("");

        ImGui::Text("Performance");
        ImGui::Separator();
        ImGui::Text("Average ms/frame: %.3f", 1000.0f / applicationState->IO->Framerate);
        ImGui::Text("FPS: %.1f ", applicationState->IO->Framerate);
        ImGui::Text("");

        ImGui::End();
    }
}


global_function void DrawGui(application_state *applicationState, application_memory *memory, processor_8086 *processor)
{
    ShowMainMenuBar(applicationState);

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    ShowDisassemblyWindow(applicationState, processor, &memory->Instructions.Arena);
    ShowRegistersWindow(applicationState, processor);
    ShowMemoryWindow(applicationState, processor);

    ShowDiagnosticsWindow(applicationState, memory, processor);

    // ImGui::ShowDemoWindow();
    // ImGui::ShowStackToolWindow();
}
