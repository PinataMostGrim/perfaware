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
global_function void *PushSize(memory_arena *arena, memory_index size);
global_function void *PushSizeZero(memory_arena *arena, memory_index size);
global_function void *PushData(memory_arena *arena, memory_index size, U8 *sourcePtr);
global_function void *PushString(memory_arena *arena, char *str);
global_function void *PopSize(memory_arena *arena, memory_index size);
global_function void ClearArena(memory_arena *arena);

#define PushStruct(arena, type) (type *)PushSize(arena, sizeof(type))
#define PushArray(arena, type, count) (type *)PushSize((arena), sizeof(type)*(count))
#define PopStruct(arena, type) (type *)PopSize(arena, sizeof(type))

#endif // MEMORY_ARENA_H
