/* TODO (Aaron):
    - Add ArenaPushFile() method?
*/

#include "base.h"
#include "base_memory.h"
#include "base_arena.h"


global_function void ArenaInitialize(memory_arena *arena, memory_index size, U8 *basePtr)
{
    arena->BasePtr = basePtr;
    arena->PositionPtr = basePtr;
    arena->Size = size;
    arena->Used = 0;
}


global_function void *ArenaPushSize(memory_arena *arena, memory_index size)
{
    Assert(((arena->Used + size) <= arena->Size) && "Attempted to allocate more space than the arena has remaining");

    void *result = arena->BasePtr + arena->Used;
    arena->Used += size;
    arena->PositionPtr = arena->BasePtr + arena->Used;

    return result;
}


global_function void *ArenaPushSizeZero(memory_arena *arena, memory_index size)
{
    Assert((arena->Used + size) <= arena->Size && "Attempted to allocate more space than the arena has remaining");

    MemorySet(arena->PositionPtr, 0, size);

    void *result = arena->BasePtr + arena->Used;
    arena->Used += size;
    arena->PositionPtr = arena->BasePtr + arena->Used;

    return result;
}


global_function void *ArenaPushData(memory_arena *arena, memory_index size, U8 *sourcePtr)
{
    void *result = ArenaPushSize(arena, size);
    MemoryCopy(result, sourcePtr, size);

    return result;
}


global_function void *ArenaPopSize(memory_arena *arena, memory_index size)
{
    Assert(arena->Used >= size && "Attempted to free more space than has been filled");

    arena->Used -= size;
    arena->PositionPtr = arena->BasePtr + arena->Used;

    void *result = arena->BasePtr + arena->Used;
    return result;
}


global_function void ArenaClear(memory_arena *arena)
{
    arena->PositionPtr = arena->BasePtr;
    arena->Used = 0;
}


global_function void ArenaClearZero(memory_arena *arena)
{
    arena->PositionPtr = arena->BasePtr;
    arena->Used = 0;

    MemorySet(arena->BasePtr, 0x0, arena->Size);
}
