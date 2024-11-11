/* TODO (Aaron):
    - Add ArenaPushFile() method?
*/

#include "base.h"
#include "base_memory.h"
#include "base_arena.h"



global_function memory_arena ArenaAllocate(size_t size, size_t maxSize)
{
    memory_arena result = {0};

    // Note (Aaron): A value of 0 for maxSize indicates the arena is not growable.
    if (!maxSize || size > maxSize)
    {
        maxSize = size;
    }

    U8 *base = (U8 *)MemoryReserve(maxSize);
    if (base)
    {
        MemoryCommit(base, size);
        ArenaInitialize(&result, size, maxSize, base);
    }

    return result;
}


global_function void ArenaInitialize(memory_arena *arena, memory_index size, memory_index maxSize, U8 *basePtr)
{
    arena->BasePtr = basePtr;
    arena->PositionPtr = basePtr;
    arena->Size = size;
    arena->MaxSize = maxSize;
    arena->Used = 0;
}


global_function B32 ArenaIsAllocated(memory_arena *arena)
{
    B32 result = arena->BasePtr != 0;
    return result;
}


global_function void *ArenaPushSize(memory_arena *arena, memory_index size)
{
    // Note (Aaron): Guard against exceeding the arena's MaxSize
    if ((arena->Used + size) > arena->MaxSize)
    {
        Assert(FALSE && "Attempted to allocate more space than the arena has remaining.");
        return 0;
    }

    // Note (Aaron): Commit extra memory if necessary.
    if (((arena->Used + size) > arena->Size)
        && (arena->Used + size) <= arena->MaxSize)
    {
        B32 success = MemoryCommit(arena->PositionPtr, size);
        if (!success)
        {
            Assert(FALSE && "Failed to commit additional memory for arena.");
            return 0;
        }

        arena->Size = arena->Used + size;
    }

    void *result = arena->BasePtr + arena->Used;
    arena->Used += size;
    arena->PositionPtr = arena->BasePtr + arena->Used;

    return result;
}


global_function void *ArenaPushSizeZero(memory_arena *arena, memory_index size)
{
    // Note (Aaron): Guard against exceeding the arena's MaxSize
    if ((arena->Used + size) > arena->MaxSize)
    {
        Assert(FALSE && "Attempted to allocate more space than the arena has remaining.");
        return 0;
    }

    // Note (Aaron): Commit extra memory if necessary.
    if (((arena->Used + size) > arena->Size)
        && (arena->Used + size) <= arena->MaxSize)
    {
        B32 success = MemoryCommit(arena->PositionPtr, size);
        if (!success)
        {
            Assert(FALSE && "Failed to commit additional memory for arena.");
            return 0;
        }

        arena->Size = arena->Used + size;
    }

    MemorySet(arena->PositionPtr, 0, size);

    void *result = arena->BasePtr + arena->Used;
    arena->Used += size;
    arena->PositionPtr = arena->BasePtr + arena->Used;

    return result;
}


global_function void *ArenaPushData(memory_arena *arena, memory_index size, U8 *sourcePtr)
{
    void *result = ArenaPushSize(arena, size);
    if (!result)
    {
        return 0;
    }

    MemoryCopy(result, sourcePtr, size);

    return result;
}


global_function void *ArenaPopSize(memory_arena *arena, memory_index size)
{
    if (arena->Used < size)
    {
        Assert(arena->Used >= size && "Attempted to free more space than has been filled");
        size = arena->Used;
    }

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
