#ifndef SIM8086_PLATFORM_H
#define SIM8086_PLATFORM_H

#include "base.h"
#include "memory_arena.h"


typedef struct
{
    void *BackingStore;
    U64 Size;
    memory_arena Arena;
    B32 IsInitialized;
} sim8086_memory;

#endif // SIM8086_PLATFORM_H
