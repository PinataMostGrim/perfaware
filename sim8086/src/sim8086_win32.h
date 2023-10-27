#ifndef WIN32_SIM8086_H
#define WIN32_SIM8086_H


#include <windows.h>

#include "base_string.h"
#include "sim8086_platform.h"
#include "sim8086_gui.h"


typedef struct
{
    Str8 FullExePath;
    Str8 ExeFolderPath;
    Str8 ExeFilename;
    Str8 DLLPath;
    Str8 DLLTempPath;
    Str8 DLLLockPath;
} win32_context;


typedef struct
{
    HMODULE CodeDLL;
    FILETIME LastWriteTime;

    set_imgui_context *SetImGuiContext;
    update_and_render *UpdateAndRender;

    B32 IsValid;
} application_code;

#endif // WIN32_SIM8086_H
