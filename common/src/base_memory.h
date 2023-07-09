#ifndef BASE_MEMORY_H
#define BASE_MEMORY_H

#include "base.h"
#include "base_types.h"


// +------------------------------+
// Note (Aaron): Memory modification

global_function void *MemorySet(void *destPtr, int c, size_t count);
global_function void *MemoryCopy(void *destPtr, void const *sourcePtr, size_t size);

#define MemoryZero(ptr, count)  MemorySet((ptr), 0, (count))
#define MemoryZeroStruct(ptr)   MemoryZero((ptr), sizeof(*(ptr)))
#define MemoryZeroArray(array)  MemoryZero((array), sizeof(array))

#define MemoryCopyStruct(destPtr, sourcePtr)    MemoryCopy((destPtr), (sourcePtr), Min(sizeof(*(destPtr)), sizeof(*(sourcePtr))))
#define MemoryCopyArray(destArray, sourceArray) MemoryCopy((destArray), (sourceArray), Min(sizeof(sourceArray), sizeof(destArray)))


// +------------------------------+
// Note (Aaron): Memory arenas

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

global_function void *ArenaPopSize(memory_arena *arena, memory_index size);
#define ArenaPopStruct(arena, type) (type *)ArenaPopSize(arena, sizeof(type))

global_function void ArenaClear(memory_arena *arena);
global_function void ArenaClearZero(memory_arena *arena);

#endif // BASE_MEMORY_H
