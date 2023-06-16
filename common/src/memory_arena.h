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


global_function void InitializeArena(memory_arena *arena, memory_index size, U8 *basePtr);
global_function void *PushSize_(memory_arena *arena, memory_index size);
global_function void *PushSizeZero_(memory_arena *arena, memory_index size);
global_function void *PopSize_(memory_arena *arena, memory_index size);

#define PushSize(arena, type) (type *)PushSize_(arena, sizeof(type))
#define PopSize(arena, type) (type *)PopSize_(arena, sizeof(type))

#endif // MEMORY_ARENA_H
