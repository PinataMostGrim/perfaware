#ifndef BASE_ARENA
#define BASE_ARENA

#include "base.h"
#include "base_types.h"


typedef size_t memory_index;

typedef struct
{
    U8 *BasePtr;
    U8 *PositionPtr;
    memory_index Size;
    memory_index MaxSize;
    memory_index Used;
} memory_arena;


global_function memory_arena ArenaAllocate(size_t size, size_t maxSize);
global_function void ArenaInitialize(memory_arena *arena, memory_index size, memory_index maxSize, U8 *basePtr);

global_function void *ArenaPushSize(memory_arena *arena, memory_index size);
global_function void *ArenaPushSizeZero(memory_arena *arena, memory_index size);

#define ArenaPushStruct(arena, type) (type *)ArenaPushSize(arena, sizeof(type))
#define ArenaPushArray(arena, type, count) (type *)ArenaPushSize((arena), sizeof(type)*(count))

global_function void *ArenaPushData(memory_arena *arena, memory_index size, U8 *sourcePtr);

global_function void *ArenaPopSize(memory_arena *arena, memory_index size);
#define ArenaPopStruct(arena, type) (type *)ArenaPopSize(arena, sizeof(type))

global_function void ArenaClear(memory_arena *arena);
global_function void ArenaClearZero(memory_arena *arena);

#endif // BASE_ARENA
