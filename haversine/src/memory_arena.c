#include "base.h"
#include "memory_arena.h"


function void InitializeArena(memory_arena *arena, memory_index size, U8 *basePtr)
{
    arena->BasePtr = basePtr;
    arena->PositionPtr = basePtr;
    arena->Size = size;
    arena->Used = 0;
}


function void *PushSize_(memory_arena *arena, memory_index size)
{
    Assert((arena->Used + size) <= arena->Size);

    void *result = arena->BasePtr + arena->Used;
    arena->Used += size;
    arena->PositionPtr = arena->BasePtr + arena->Used;

    return result;
}


function void *PopSize_(memory_arena *arena, memory_index size)
{
    Assert(arena->Used >= size);

    arena->Used -= size;
    arena->PositionPtr = arena->BasePtr + arena->Used;

    void *result = arena->BasePtr + arena->Used;
    return result;
}


#define PushSize(arena, type) (type *)PushSize_(arena, sizeof(type))
#define PopSize(arena, type) (type *)PopSize_(arena, sizeof(type))