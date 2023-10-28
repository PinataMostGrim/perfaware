#ifndef SIM8086_NIX_H
#define SIM8086_NIX_H

#include "base_string.h"
#include "sim8086_platform.h"
#include "sim8086_gui.h"


typedef struct
{
    Str8 FullExePath;
    Str8 ExeFolderPath;
    Str8 ExeFilename;
    Str8 SOPath;
    Str8 SOTempPath;
    Str8 SOLockPath;
} linux_context;


typedef struct
{
    void *CodeSO;
    U64 LastWriteTime;

    set_imgui_context *SetImGuiContext;
    update_and_render *UpdateAndRender;

    B32 IsValid;
} application_code;

#endif // SIM8086_NIX_H
