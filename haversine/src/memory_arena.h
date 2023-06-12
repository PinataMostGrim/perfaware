#ifndef MEMORY_ARENA_H
#define MEMORY_ARENA_H

#include "base.h"


typedef size_t memory_index;

typedef struct
{
    U8 *BasePtr;
    U8 *PositionPtr;
    memory_index Size;
    memory_index Used;
} memory_arena;


function void InitializeArena(memory_arena *arena, memory_index size, U8 *basePtr);
function void *PushSize_(memory_arena *arena, memory_index size);
function void *PopSize_(memory_arena *arena, memory_index size);

#endif // MEMORY_ARENA_H