/* TODO (Aaron):
    - Update PushString() to make use of length based strings once I implement them.
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


global_function void *PushSize(memory_arena *arena, memory_index size)
{
    Assert(((arena->Used + size) <= arena->Size) && "Attempted to allocate more space than the arena has remaining");

    void *result = arena->BasePtr + arena->Used;
    arena->Used += size;
    arena->PositionPtr = arena->BasePtr + arena->Used;

    return result;
}


global_function void *PushSizeZero(memory_arena *arena, memory_index size)
{
    Assert((arena->Used + size) <= arena->Size && "Attempted to allocate more space than the arena has remaining");

    MemorySet(arena->PositionPtr, 0, size);

    void *result = arena->BasePtr + arena->Used;
    arena->Used += size;
    arena->PositionPtr = arena->BasePtr + arena->Used;

    return result;
}


global_function void *PushData(memory_arena *arena, memory_index size, U8 *sourcePtr)
{
    void *result = PushSize(arena, size);
    MemoryCopy(result, sourcePtr, size);

    return result;
}


global_function void *PushString(memory_arena *arena, char *str)
{
    U64 strLength = GetStringLength(str);
    void *result = PushSize(arena, strLength);
    MemoryCopy(result, str, strLength);

    return result;
}


global_function void *PopSize(memory_arena *arena, memory_index size)
{
    Assert(arena->Used >= size && "Attempted to free more space than has been filled");

    arena->Used -= size;
    arena->PositionPtr = arena->BasePtr + arena->Used;

    void *result = arena->BasePtr + arena->Used;
    return result;
}


global_function void ClearArena(memory_arena *arena)
{
    arena->PositionPtr = arena->PositionPtr;
    arena->Used = 0;
}
