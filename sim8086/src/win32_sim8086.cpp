/* TODO (Aaron):
    - Add support for hot-reload of GUI code
        - Setup function pointers
        - Include all the code required for renaming the original DLL, etc
        - Update the batch script to create a lock file and then delete it
    - Add a screen to display loaded instructions
    - Add a screen to display registers
    - Add a screen to display memory
    - Add hotkeys for step forward through assembly
    - Give processor or file error feedback via GUI
    - Add a file menu to open / load files
        - Use hard-coded value for now
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
#include "memory.h"
#include "memory_arena.c"

#include "sim8086_platform.h"
#include "sim8086_gui.h"
#include "sim8086.cpp"

struct WGL_WindowData { HDC hDC; };

global_variable HGLRC            g_hRC;
global_variable WGL_WindowData   g_MainWindow;
global_variable int              g_Width;
global_variable int              g_Height;


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Note (Aaron): Consider these these values for adjustment
global_variable const char * ASSEMBLY_FILE = "..\\listings\\listing_0037_single_register_mov";
global_variable U32 PERMANENT_MEMORY_SIZE = Megabytes(2);
global_variable U32 FILE_PATH_BUFFER_SIZE = Kilobytes(1);

global_variable ImVec4 CLEAR_COLOR = {0.45f, 0.55f, 0.60f, 1.00};


typedef struct
{
    char *FullExePath;
    char *ExeFolderPath;
    char *ExeFilename;
} win32_context;


global_function void Win32GetExeInfo(win32_context *context, memory_arena *arena)
{
    // TODO (Aaron): Make use of PushSizeZero() in this function once it has been implemented
    // so we can eliminate the call to set the null terminator.

    context->FullExePath = (char *)PushSize_(arena, FILE_PATH_BUFFER_SIZE);
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

    U64 sizeOfFolderPath = lastSlashPlusOne - context->FullExePath;
    context->ExeFolderPath = (char *)PushSize_(arena, sizeOfFolderPath + 1);     // + 1 for null termination character
    if (sizeOfFolderPath > 0)
    {
        MemoryCopy(context->ExeFolderPath, context->FullExePath, sizeOfFolderPath);
        // Note (Aaron): Set the null terminator in case the memory hasn't been initialized to 0
        MemorySet((U8 *)context->ExeFolderPath + sizeOfFolderPath, 0x0, 1);
    }

    U64 sizeOfFilename = sizeOfFullPath - sizeOfFolderPath;
    Assert(sizeOfFilename > 0 && "Filename cannot be 0 characters in length");
    context->ExeFilename = (char *)PushSize_(arena, sizeOfFilename + 1);         // + 1 for null termination character
    MemoryCopy(context->ExeFilename, context->FullExePath + sizeOfFolderPath, sizeOfFilename);
    // Note (Aaron): Set the null terminator in case the memory hasn't been initialized to 0
    MemorySet((U8 *)context->ExeFilename + sizeOfFilename, 0xff, 1);
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
    // create window and register
    WNDCLASSA windowClass = {};

    windowClass.style = CS_HREDRAW|CS_VREDRAW;
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = Instance;
    windowClass.hCursor = LoadCursor(0, IDC_ARROW);
    // windowClass.hIcon = ;
    windowClass.lpszClassName = "win32sim8086";

    if (!RegisterClassA(&windowClass))
    {
        // TODO (Aaron): Log error and exit
        return 1;
    }

    HWND window =
        CreateWindowExA(
            0,
            windowClass.lpszClassName,
            "win32sim8086",
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
        // TODO (Aaron): Log error and exit
        return 1;
    }

    // initialize OpenGL
    if (!CreateDeviceWGL(window, &g_MainWindow))
    {
        CleanupDeviceWGL(window, &g_MainWindow);
        DestroyWindow(window);
        UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);

        return 1;
    }

    wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    // show the window
    ShowWindow(window, SW_SHOWDEFAULT);

    // setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGuiContext *guiContext = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
    SetImguiContext(guiContext);

    // Setup Dear ImGui Style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_InitForOpenGL(window);
    ImGui_ImplOpenGL3_Init();

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

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;

    // allocate memory for a memory arena
    sim8086_memory memory = {};
    memory.Size = PERMANENT_MEMORY_SIZE;
    memory.BackingStore = VirtualAlloc(0, memory.Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (!memory.BackingStore)
    {
        Assert(FALSE && "Unable to allocate permanent memory");
        return 1;
    }
    InitializeArena(&memory.Arena, memory.Size, (U8 *)memory.BackingStore);
    memory.IsInitialized = TRUE;

    // initialize processor
    processor_8086 processor = {};
    processor.Memory = (U8 *)PushSize_(&memory.Arena, processor.MemorySize);
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


    // get exe file name and path to prep for hot-loading
    win32_context context = {};
    Win32GetExeInfo(&context, &memory.Arena);


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

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        DrawGui(&io, &show_demo_window, &show_another_window, &CLEAR_COLOR, &processor);

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
