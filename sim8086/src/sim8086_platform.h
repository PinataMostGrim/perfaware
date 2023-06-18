#ifndef SIM8086_PLATFORM_H
#define SIM8086_PLATFORM_H

#include "base.h"
#include "memory_arena.h"


typedef struct
{
    void *BackingStore;
    U64 TotalSize;

    memory_arena PermanentArena;
    memory_arena FrameArena;
    memory_arena InstructionsArena;

    B32 IsInitialized;
} application_memory;

#endif // SIM8086_PLATFORM_H
