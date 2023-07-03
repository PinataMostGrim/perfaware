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


global_function void ArenaInitialize(memory_arena *arena, memory_index size, U8 *basePtr);

global_function void *ArenaPushSize(memory_arena *arena, memory_index size);
global_function void *ArenaPushSizeZero(memory_arena *arena, memory_index size);

#define ArenaPushStruct(arena, type) (type *)ArenaPushSize(arena, sizeof(type))
#define ArenaPushArray(arena, type, count) (type *)ArenaPushSize((arena), sizeof(type)*(count))

global_function void *ArenaPushData(memory_arena *arena, memory_index size, U8 *sourcePtr);
global_function char *ArenaPushString(memory_arena *arena, char *str);
global_function char *ArenaPushStringConcat(memory_arena *arena, char *str);

global_function void *ArenaPopSize(memory_arena *arena, memory_index size);
#define ArenaPopStruct(arena, type) (type *)ArenaPopSize(arena, sizeof(type))

global_function void ArenaClear(memory_arena *arena);
global_function void ArenaClearZero(memory_arena *arena);

#endif // MEMORY_ARENA_H
