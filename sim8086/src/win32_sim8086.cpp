/* TODO (Aaron):

    - Need to add file loading to platform layer so that the application can load assemblies into the processor
    - Add a ScratchArena when I need it
    - Convert to length based strings

    - Add feedback to main window titlebar (loaded program name, etc)
    - Add a screen to display memory
    - Add instruction bits when hovering over an assembly line
    - Add hotkeys for step forward through assembly
    - Give processor or file error feedback via GUI
    - Add a file menu to open / load files
        - Use hard-coded value for now
    - Add V2F32 (and others from base.h) macros to imconfig.h so I can use my own math types
*/

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <windows.h>
#include <GL/GL.h>
#include <tchar.h>

#include "base.h"
#include "memory_arena.h"
#include "win32_sim8086.h"
#include "sim8086_platform.h"
#include "sim8086_gui.h"
#include "sim8086.h"
#include "sim8086_mnemonics.h"

#include "memory_arena.c"
#include "sim8086.cpp"
#include "sim8086_mnemonics.cpp"


// ///////////////////////////////////////////////////////////////
// Note (Aaron): Adjustable values
// global_variable const char * ASSEMBLY_FILE = "..\\listings\\listing_0037_single_register_mov";
// global_variable const char * ASSEMBLY_FILE = "..\\listings\\listing_0039_more_movs";
global_variable const char * ASSEMBLY_FILE = "..\\listings\\listing_0041_add_sub_cmp_jnz";

// Note (Aaron): Update the build batch file when adjusting these three
global_variable char *DLL_FILENAME = (char *)"sim8086_application.dll";
global_variable char *DLL_TEMP_FILENAME = (char *)"sim8086_application_temp.dll";
global_variable char *DLL_LOCK_FILENAME = (char *)"sim8086_lock.tmp";

global_variable U64 PERMANENT_ARENA_SIZE = Megabytes(10);
global_variable U64 INSTRUCTION_ARENA_SIZE = Megabytes(10);
global_variable U64 FRAME_ARENA_SIZE = Megabytes(10);

global_variable U32 FILE_PATH_BUFFER_SIZE = 512;

global_variable ImVec4 CLEAR_COLOR = {0.45f, 0.55f, 0.60f, 1.00};
// ///////////////////////////////////////////////////////////////


struct WGL_WindowData { HDC hDC; };

global_variable HGLRC            g_hRC;
global_variable WGL_WindowData   g_MainWindow;
global_variable int              g_Width;
global_variable int              g_Height;


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


global_function char *ConcatStrings(char *stringA, char *stringB, memory_arena *arena)
{
    U64 sizeOfA = GetStringLength(stringA);
    U64 sizeOfB = GetStringLength(stringB);
    U64 resultSize = sizeOfA + sizeOfB;

    char *result = (char *)PushSizeZero(arena, resultSize + 1);
    MemoryCopy(result, stringA, sizeOfA);
    MemoryCopy(result + sizeOfA, stringB, sizeOfB);

    return result;
}


global_function void Win32GetExeInfo(win32_context *context, memory_arena *arena)
{
    // extract full path
    context->FullExePath = (char *)PushSizeZero(arena, FILE_PATH_BUFFER_SIZE);
    DWORD sizeOfFullPath = GetModuleFileName(0, context->FullExePath, FILE_PATH_BUFFER_SIZE);
    Assert((sizeOfFullPath < FILE_PATH_BUFFER_SIZE) && "Allocated a buffer that was too small for the length of the file path");

    char *lastSlashPlusOne = context->FullExePath;
    for (char *scan = context->FullExePath; *scan; ++scan)
    {
        if (*scan == '\\')
        {
            lastSlashPlusOne = scan + 1;
        }
    }

    // construct exe folder path
    U64 sizeOfFolderPath = lastSlashPlusOne - context->FullExePath;
    context->ExeFolderPath = (char *)PushSizeZero(arena, sizeOfFolderPath + 1);     // + 1 for null termination character
    if (sizeOfFolderPath > 0)
    {
        MemoryCopy(context->ExeFolderPath, context->FullExePath, sizeOfFolderPath);
    }

    // construct exe filename
    U64 sizeOfFilename = sizeOfFullPath - sizeOfFolderPath;
    Assert(sizeOfFilename > 0 && "Filename cannot be 0 characters in length");
    context->ExeFilename = (char *)PushSizeZero(arena, sizeOfFilename + 1);         // + 1 for null termination character
    MemoryCopy(context->ExeFilename, context->FullExePath + sizeOfFolderPath, sizeOfFilename);

    // construct GUI DLL related paths
    context->DLLPath = ConcatStrings(context->ExeFolderPath, DLL_FILENAME, arena);
    context->DLLTempPath = ConcatStrings(context->ExeFolderPath, DLL_TEMP_FILENAME, arena);
    context->DLLLockPath = ConcatStrings(context->ExeFolderPath, DLL_LOCK_FILENAME, arena);
}


global_function FILETIME Win32GetLastWriteTime(char *filename)
{
    FILETIME lastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesEx(filename, GetFileExInfoStandard, &data))
    {
        lastWriteTime = data.ftLastWriteTime;
    }

    return lastWriteTime;
}


global_function application_code Win32LoadAppCode(char *sourceDLLPath, char *tempDLLPath, char *lockFilePath)
{
    application_code result = {};

    // Note (Aaron): Not necessary if using a debugger that reloads PDB files
#if 0
    // early out if the PDB lock file is present
    WIN32_FILE_ATTRIBUTE_DATA lockFileData;
    if (GetFileAttributesEx(lockFilePath, GetFileExInfoStandard, &lockFileData))
    {
        return result;
    }
#endif

    // re-assign function pointers
    result.LastWriteTime = Win32GetLastWriteTime(sourceDLLPath);
    CopyFileA(sourceDLLPath, tempDLLPath, FALSE);
    result.CodeDLL = LoadLibraryA(tempDLLPath);
    if (result.CodeDLL)
    {
        result.SetImGuiContext = (set_imgui_context *)GetProcAddress(result.CodeDLL, "SetImGuiContext");
        result.UpdateAndRender = (update_and_render *)GetProcAddress(result.CodeDLL, "UpdateAndRender");

        result.IsValid = (result.UpdateAndRender && result.SetImGuiContext);
    }

    if(!result.IsValid)
    {
        result.SetImGuiContext = 0;
        result.UpdateAndRender = 0;
    }

    return result;
}


global_function void Win32UnloadCode(application_code *applicationCode)
{
    if (applicationCode->CodeDLL)
    {
        FreeLibrary(applicationCode->CodeDLL);
        applicationCode->CodeDLL = 0;
    }

    applicationCode->IsValid = FALSE;
    applicationCode->SetImGuiContext = 0;
    applicationCode->UpdateAndRender = 0;
}


// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }

    switch (msg)
    {
        case WM_SIZE:
        {
            if (wParam != SIZE_MINIMIZED)
            {
                g_Width = LOWORD(lParam);
                g_Height = HIWORD(lParam);
            }
            return 0;
        }
        case WM_SYSCOMMAND:
        {
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            {
                return 0;
            }
            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}


global_function bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    HDC hDc = GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    const int pf = ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;
    ReleaseDC(hWnd, hDc);

    data->hDC = GetDC(hWnd);
    if (!g_hRC)
        g_hRC = wglCreateContext(data->hDC);
    return true;
}


global_function void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    wglMakeCurrent(NULL, NULL);
    ReleaseDC(hWnd, data->hDC);
}


int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CommandLine,
    int ShowCode)
{
    WNDCLASSA windowClass = {};
    windowClass.style = CS_HREDRAW|CS_VREDRAW;
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = Instance;
    windowClass.hCursor = LoadCursor(0, IDC_ARROW);
    windowClass.lpszClassName = "win32_sim8086";

    if (!RegisterClassA(&windowClass))
    {
        Assert(FALSE && "Unable to register window class");
        return 1;
    }

    HWND window =
        CreateWindowExA(
            0,
            windowClass.lpszClassName,
            "win32_sim8086",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0);

    if (!window)
    {
        Assert(FALSE && "Unable to create Window");
        return 1;
    }


    // allocate application memory
    application_memory memory = {};
    memory.TotalSize = PERMANENT_ARENA_SIZE + INSTRUCTION_ARENA_SIZE + FRAME_ARENA_SIZE;
    memory.BackingStore = VirtualAlloc(0, memory.TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (!memory.BackingStore)
    {
        Assert(FALSE && "Unable to allocate permanent memory");
        return 1;
    }

    // partition out memory arenas
    {
        InitializeArena(&memory.PermanentArena, PERMANENT_ARENA_SIZE, (U8 *)memory.BackingStore);

        U8 *instructionsArenaPtr = ((U8 *)memory.BackingStore) + memory.PermanentArena.Size;
        InitializeArena(&memory.InstructionsArena, INSTRUCTION_ARENA_SIZE, instructionsArenaPtr);

        U8 *frameArenaPtr = ((U8 *)memory.BackingStore) + memory.PermanentArena.Size + memory.InstructionsArena.Size;
        InitializeArena(&memory.FrameArena, FRAME_ARENA_SIZE, frameArenaPtr);

        memory.IsInitialized = (memory.PermanentArena.BasePtr && memory.InstructionsArena.BasePtr && memory.FrameArena.BasePtr);
    }


    // construct paths in prep for GUI code hot-loading
    win32_context win32Context = {};
    Win32GetExeInfo(&win32Context, &memory.PermanentArena);

    // initial hot-load of application code
    application_code applicationCode = Win32LoadAppCode(win32Context.DLLPath, win32Context.DLLTempPath, win32Context.DLLLockPath);
    if (!applicationCode.IsValid)
    {
        Assert(FALSE && "Unable to perform initial hot-load of GUI code");
        return 1;
    }


    // initialize OpenGL
    if (!CreateDeviceWGL(window, &g_MainWindow))
    {
        CleanupDeviceWGL(window, &g_MainWindow);
        DestroyWindow(window);
        UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);

        Assert(FALSE && "OpenGL failed to initialize");
        return 1;
    }

    wglMakeCurrent(g_MainWindow.hDC, g_hRC);
    ShowWindow(window, SW_SHOWDEFAULT);


    // setup Dear ImGui context and gui state
    IMGUI_CHECKVERSION();
    ImGuiContext *guiContext = ImGui::CreateContext();
    // TODO (Aaron): I don't understand the syntax in these lines
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsResizeFromEdges = true;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_InitForOpenGL(window);
    ImGui_ImplOpenGL3_Init();

    if (applicationCode.SetImGuiContext) applicationCode.SetImGuiContext(guiContext);


    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);


    // initialize 8086
    processor_8086 processor = {};
    processor.Memory = (U8 *)PushSize(&memory.PermanentArena, processor.MemorySize);
    if (!processor.Memory)
    {
        Assert(FALSE && "Unable to allocate main memory for 8086");
        return 1;
    }

    // load program into the processor
    FILE *file = {};
    file = fopen(ASSEMBLY_FILE, "rb");
    if (!file)
    {
        Assert(FALSE && "Unable to load hard-coded assembly file");
        return 1;
    }

    // load program into 8086
    processor.ProgramSize = (U16)fread(processor.Memory, 1, processor.MemorySize, file);

    // error handling for file read
    if (ferror(file))
    {
        Assert(FALSE && "Encountered error while reading file");
        return 1;
    }
    if (!feof(file))
    {
        Assert(FALSE && "Program size exceeds processor memory; unable to load");
        return 1;
    }

    fclose(file);

    // generate instructions from program
    instruction *instructionBuffer = (instruction *)memory.InstructionsArena.BasePtr;
    while (processor.IP < processor.ProgramSize)
    {
        instruction nextInstruction = DecodeNextInstruction(&processor);
        instruction *nextInstructionPtr = PushStruct(&memory.InstructionsArena, instruction);
        MemoryCopy(nextInstructionPtr, &nextInstruction, sizeof(instruction));
    }

    processor.IP = 0;

    application_state applicationState = {};
    applicationState.IO = &io;
    applicationState.ClearColor = CLEAR_COLOR;
    applicationState.Assembly_SelectedLine = 0;


    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        MSG msg;
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
// Note (Aaron): Debug hotkey for closing the application
#if 1
            if (msg.message == WM_KEYUP)
            {
                U32 vKCode = (U32)msg.wParam;
                if (vKCode == VK_ESCAPE)
                {
                    done = true;
                }
            }
#endif

            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                done = true;
            }
        }

        if (done)
        {
            break;
        }

        ClearArena(&memory.FrameArena);

        // Hot-load code if necessary
        FILETIME dllWriteTime = Win32GetLastWriteTime(win32Context.DLLPath);
        if (CompareFileTime(&dllWriteTime, &applicationCode.LastWriteTime))
        {
            Win32UnloadCode(&applicationCode);
            // TODO (Aaron): Why was it 75 specifically?
            Sleep(75);
            applicationCode = Win32LoadAppCode(win32Context.DLLPath, win32Context.DLLTempPath, win32Context.DLLLockPath);
            if (applicationCode.SetImGuiContext) applicationCode.SetImGuiContext(guiContext);
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (applicationCode.UpdateAndRender)
        {
            applicationCode.UpdateAndRender(&applicationState, &memory, &processor);
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, g_Width, g_Height);
        glClearColor(CLEAR_COLOR.x, CLEAR_COLOR.y, CLEAR_COLOR.z, CLEAR_COLOR.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Present
        SwapBuffers(g_MainWindow.hDC);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext(guiContext);

    CleanupDeviceWGL(window, &g_MainWindow);
    wglDeleteContext(g_hRC);
    DestroyWindow(window);
    UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);

    return 0;
}
