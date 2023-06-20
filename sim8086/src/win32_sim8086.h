#ifndef WIN32_SIM8086_H
#define WIN32_SIM8086_H


#include <windows.h>

#include "sim8086_platform.h"
#include "sim8086_gui.h"


typedef struct
{
    char *FullExePath;
    char *ExeFolderPath;
    char *ExeFilename;
    char *DLLPath;
    char *DLLTempPath;
    char *DLLLockPath;
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
