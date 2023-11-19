#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"

#include "imgui_impl_opengl3.cpp"
#include "imgui_impl_glfw.cpp"

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

#include <GLFW/glfw3.h>

#include "base_inc.h"
#include "sim8086_linux.h"
#include "sim8086_platform.h"
#include "sim8086.h"
#include "sim8086_mnemonics.h"
#include "sim8086_gui.h"

#include "base_types.c"
#include "base_memory.c"
#include "base_string.c"
#include "sim8086.cpp"
#include "sim8086_mnemonics.cpp"


// Note (Aaron): Adjustable values
// ///////////////////////////////////////////////////////////////

// Note (Aaron): Update the build script when adjusting these three
global_variable char *SO_FILENAME = (char *)"sim8086_application.so";
global_variable char *SO_TEMP_FILENAME = (char *)"sim8086_application_temp.so";
global_variable char *SO_LOCK_FILENAME = (char *)"sim8086_lock.tmp";

global_variable ImVec4 CLEAR_COLOR = {0.45f, 0.55f, 0.60f, 1.00};
// ///////////////////////////////////////////////////////////////


static void GLFW_ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}


global_function void LinuxGetExeInfo(linux_context *context, memory_arena *arena)
{
    char *buffer;
    memory_index bufferSize = 128;
    memory_index sizeOfFullPath = 0;

    for (;;)
    {
        buffer = (char *)ArenaPushSize(arena, bufferSize);
        sizeOfFullPath = readlink("/proc/self/exe", buffer, bufferSize);

        if (sizeOfFullPath == bufferSize)
        {
            ArenaPopSize(arena, bufferSize);
            bufferSize *= 2;
            continue;
        }

        break;
    }

    memory_index unused = bufferSize - sizeOfFullPath;
    ArenaPopSize(arena, unused);
    Str8 fullExePath = String8((U8 *)buffer, sizeOfFullPath);
    context->FullExePath = fullExePath;

    // Note (Aaron): Append the null-terminator character
    ArenaPushSizeZero(arena, 1);

    // construct exe folder path
    U32 lastSlash = 0;
    U8 *scan = fullExePath.Str;
    for (U32 i = 0; i < fullExePath.Length; ++i)
    {
        scan = fullExePath.Str + i;
        if (*scan == '/')
        {
            lastSlash = i;
        }
    }
    Str8 exeFolderPath = String8(fullExePath.Str, lastSlash + 1);
    context->ExeFolderPath = ArenaPushStr8Copy(arena, exeFolderPath, TRUE);

    // construct exe filename
    U64 sizeOfFilename = fullExePath.Length - exeFolderPath.Length;
    Assert(sizeOfFilename > 0 && "Filename cannot be 0 characters in length");
    Str8 exeFilename = String8(fullExePath.Str + exeFolderPath.Length, sizeOfFilename);
    context->ExeFilename = ArenaPushStr8Copy(arena, exeFilename, TRUE);

    // construct GUI DLL related paths
    Str8 soFilename = String8((U8 *)SO_FILENAME, GetStringLength(SO_FILENAME));
    Str8 soTempFilename = String8((U8 *)SO_TEMP_FILENAME, GetStringLength(SO_TEMP_FILENAME));
    Str8 soLockFilenameTempFilename = String8((U8 *)SO_LOCK_FILENAME, GetStringLength(SO_LOCK_FILENAME));

    context->SOPath = ConcatStr8(arena, context->ExeFolderPath, soFilename, TRUE);
    context->SOTempPath = ConcatStr8(arena, context->ExeFolderPath, soTempFilename, TRUE);
    context->SOLockPath = ConcatStr8(arena, context->ExeFolderPath, soLockFilenameTempFilename, TRUE);
}


global_function U64 LinuxGetLastWriteTime(char *filename)
{
    U64 lastWriteTime = 0;

    struct stat fileInfo;
    stat(filename, &fileInfo);
    lastWriteTime = fileInfo.st_mtim.tv_sec;

    return lastWriteTime;
}


global_function application_code LinuxLoadAppCode(char *soSourcePath, char *soTempPath, char *lockFilePath)
{
    application_code result = {};

    // Check for the lock file and early out if it exists as we have not finished compiling
    // and writing out the application code.
    int lockFileExists = access(lockFilePath, F_OK);

    // Note (Aaron): access() returns 0 if the file exists, -1 if it doesn't.
    if (lockFileExists == 0)
    {
        result.SetImGuiContext = 0;
        result.UpdateAndRender = 0;

        return result;
    }

    // gather source library information
    struct stat sourceFileInfo;
    stat(soSourcePath, &sourceFileInfo);
    result.LastWriteTime = sourceFileInfo.st_mtim.tv_sec;

    // make temp copy of application library
    int sourceFile = open(soSourcePath, O_RDONLY);
    if (!sourceFile)
    {
        fprintf(stderr, "[ERROR] Unable to open file \"%s\"\n", soSourcePath);
        return result;
    }

    int tempFile = creat(soTempPath, 0770);
    if (!tempFile)
    {
        close(sourceFile);
        fprintf(stderr, "[ERROR] Unable to open file \"%s\"\n", soTempPath);
        return result;
    }

    size_t bytesCopied;
    bytesCopied = sendfile(tempFile, sourceFile, 0, sourceFileInfo.st_size);
    close(sourceFile);
    close(tempFile);

    // Note (Aaron): sendfile() isn't guarenteed to send all bytes for some reason. If we ever want
    // to make this more robust, we could do one of the following:
    //  - try the send operation multiple times here
    //  - try using copy_file_range()
    //  - copy the file byte for byte manually
    if (sourceFileInfo.st_size != bytesCopied)
    {
        fprintf(stderr, "[ERROR] Unable to copy source SO \"%s\" path \"%s\"\n", soTempPath, soTempPath);
        return result;
    }

    // re-assign function pointers
    result.CodeSO = dlopen(soTempPath, RTLD_NOW);
    if (result.CodeSO)
    {
        result.SetImGuiContext = (set_imgui_context *)dlsym(result.CodeSO, "SetImGuiContext");
        result.UpdateAndRender = (update_and_render *)dlsym(result.CodeSO, "UpdateAndRender");

        result.IsValid = (result.SetImGuiContext && result.UpdateAndRender);
    }

    if (!result.IsValid)
    {
        result.SetImGuiContext = 0;
        result.UpdateAndRender = 0;
    }

    return result;
}


global_function void LinuxUnloadAppCode(application_code *applicationCode)
{
    if (applicationCode->CodeSO)
    {
        dlclose(applicationCode->CodeSO);
        applicationCode->CodeSO = 0;
    }

    applicationCode->IsValid = FALSE;
    applicationCode->SetImGuiContext = 0;
    applicationCode->UpdateAndRender = 0;
}


int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s [assembly filename]\n", argv[0]);
        return 1;
    }

    // Check that assembly program file exists
    const char *assemblyFilename = argv[1];
    int lockFileExists = access(assemblyFilename, F_OK);
    // Note (Aaron): access() returns 0 if the file exists, -1 if it doesn't.
    if (lockFileExists == -1)
    {
        fprintf(stderr, "Open file \"%s\" failed with error %d %s\n", assemblyFilename, errno, strerror(errno));
        return 1;
    }

    glfwSetErrorCallback(GLFW_ErrorCallback);
    if (!glfwInit())
    {
        printf("GLFW failed to initialize\n");
        return 1;
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "Example Window", nullptr, nullptr);
    if (window == nullptr)
    {
        printf("Unable to create GLFW window\n");
        return 1;
    }

    // allocate application memory
    application_memory memory = {};
    for (int i = 0; i < ArrayCount(memory.Defs); ++i)
    {
        memory.TotalSize += memory.Defs[i].Size;
    }

    memory.BackingStore = mmap(NULL, memory.TotalSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory.BackingStore == (void *)-1)
    {
        Assert(FALSE && "Unable to allocate permanent memory");
        return 1;
    }

    // partition out memory arenas
    U8 *arenaStartPtr = (U8 *)memory.BackingStore;
    memory.IsInitialized = TRUE;
    for (int i = 0; i < ArrayCount(memory.Defs); ++i)
    {
        ArenaInitialize(&memory.Defs[i].Arena, memory.Defs[i].Size, arenaStartPtr);
        arenaStartPtr += memory.Defs[i].Size;

        if (!memory.Defs[i].Arena.BasePtr)
        {
            memory.IsInitialized = FALSE;
        }
    }

    // construct paths in prep for application code hot-loading
    linux_context linuxContext = {};
    LinuxGetExeInfo(&linuxContext, &memory.Permanent.Arena);

    // initial hot-load of application code
    application_code applicationCode = LinuxLoadAppCode((char *)linuxContext.SOPath.Str,
                                                        (char *)linuxContext.SOTempPath.Str,
                                                        (char *)linuxContext.SOLockPath.Str);
    if (!applicationCode.IsValid)
    {
        Assert(FALSE && "Unable to perform initial hot-load of GUI code");
        return 1;
    }

    // initialize glfw
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);    // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGuiContext *guiContext = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsResizeFromEdges = true;
    ImGui::StyleColorsDark();

    if (applicationCode.SetImGuiContext) applicationCode.SetImGuiContext(guiContext);

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);


    // initialize 8086
    processor_8086 processor = {};
    processor.Memory = (U8 *)ArenaPushSize(&memory.Permanent.Arena, processor.MemorySize);
    if (!processor.Memory)
    {
        Assert(FALSE && "Unable to allocate main memory for 8086");
        return 1;
    }


    // initialize state
    application_state applicationState = {};
    applicationState.AssemblyFilename = assemblyFilename;
    applicationState.IO = &io;
    applicationState.ClearColor = CLEAR_COLOR;
    applicationState.Memory_StartAddress = 0;

#if SIM8086_DIAGNOSTICS
    applicationState.Diagnostics_ShowWindow = true;
#endif

    // Main loop
    // bool done = FALSE;
    // while (!done)
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // TODO (Aaron): Add debut hotkey for closing the application

        Assert(memory.Scratch.Arena.BasePtr == memory.Scratch.Arena.PositionPtr && "Scratch arena has not beel cleared");

        // Hotload code if necessary
        U64 soWriteTime = LinuxGetLastWriteTime((char *)linuxContext.SOPath.Str);
        if (soWriteTime != applicationCode.LastWriteTime)
        {
            LinuxUnloadAppCode(&applicationCode);
            applicationCode = LinuxLoadAppCode((char *)linuxContext.SOPath.Str,
                                               (char *)linuxContext.SOTempPath.Str,
                                               (char *)linuxContext.SOLockPath.Str);
            if (applicationCode.SetImGuiContext) applicationCode.SetImGuiContext(guiContext);
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (applicationCode.UpdateAndRender)
        {
            applicationCode.UpdateAndRender(&applicationState, &memory, &processor);
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(CLEAR_COLOR.x * CLEAR_COLOR.w, CLEAR_COLOR.y * CLEAR_COLOR.w, CLEAR_COLOR.z * CLEAR_COLOR.w, CLEAR_COLOR.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
