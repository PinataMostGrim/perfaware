/* TODO (Aaron):
    - Add run-time handling for when assertions would fail
        - What behaviour would we expect?
        - Return a null pointer?
        - Casey mentioned that he always returns a stub that can be used but is zeroed out every frame
    - Update PushString() to make use of length based strings once I implement them.
*/

#include "base.h"
#include "memory_arena.h"


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


global_function char *_ArenaPushString(memory_arena *arena, char *str, B8 concat)
{
    // Note (Aaron): When concatenating, we don't include the null terminating character.
    U64 strLength = GetStringLength(str) + (concat ? 0 : 1);
    char *result = (char *)ArenaPushSize(arena, strLength);
    MemoryCopy(result, str, strLength);

    return result;
}


global_function char *ArenaPushString(memory_arena *arena, char *str)
{
    return _ArenaPushString(arena, str, FALSE);
}


global_function char *ArenaPushStringConcat(memory_arena *arena, char *str)
{
    return _ArenaPushString(arena, str, TRUE);
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
    // TODO (Aaron): Test this
    arena->PositionPtr = arena->BasePtr;
    arena->Used = 0;
}


global_function void ArenaClearZero(memory_arena *arena)
{
    // TODO (Aaron): Test this
    arena->PositionPtr = arena->BasePtr;
    arena->Used = 0;

    MemorySet(arena->BasePtr, 0x0, arena->Size);
}
