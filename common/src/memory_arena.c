/* TODO (Aaron):
*/

#include "base.h"
#include "memory_arena.h"


global_function void InitializeArena(memory_arena *arena, memory_index size, U8 *basePtr)
{
    arena->BasePtr = basePtr;
    arena->PositionPtr = basePtr;
    arena->Size = size;
    arena->Used = 0;
}


global_function void *PushSize_(memory_arena *arena, memory_index size)
{
    Assert(((arena->Used + size) <= arena->Size) && "Attempted to allocate more space than the arena has remaining");

    void *result = arena->BasePtr + arena->Used;
    arena->Used += size;
    arena->PositionPtr = arena->BasePtr + arena->Used;

    return result;
}


global_function void *PushSizeZero_(memory_arena *arena, memory_index size)
{
    Assert((arena->Used + size) <= arena->Size && "Attempted to allocate more space than the arena has remaining");

    MemorySet(arena->PositionPtr, 0, size);

    void *result = arena->BasePtr + arena->Used;
    arena->Used += size;
    arena->PositionPtr = arena->BasePtr + arena->Used;

    return result;
}


global_function void *PopSize_(memory_arena *arena, memory_index size)
{
    Assert(arena->Used >= size && "Attempted to free more space than has been filled");

    arena->Used -= size;
    arena->PositionPtr = arena->BasePtr + arena->Used;

    void *result = arena->BasePtr + arena->Used;
    return result;
}
