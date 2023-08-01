/* TODO (Aaron):
    - Add run-time handling for when assertions would fail
        - What behaviour would we expect?
        - Return a null pointer?
        - Casey mentioned that he always returns a stub that can be used but is zeroed out every frame
    - Add ArenaPushFile() method?
*/

#include "base.h"
#include "base_memory.h"


global_function void *MemorySet(void *destPtr, int c, size_t count)
{
    Assert(count > 0 && "Attempted to set 0 bytes");

    unsigned char *dest = (unsigned char *)destPtr;
    while(count--) *dest++ = (unsigned char)c;

    return destPtr;
}


global_function void *MemoryCopy(void *destPtr, void const *sourcePtr, size_t size)
{
    Assert(size > 0 && "Attempted to copy 0 bytes");

    unsigned char *source = (unsigned char *)sourcePtr;
    unsigned char *dest = (unsigned char *)destPtr;
    while(size--) *dest++ = *source++;

    return destPtr;
}


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
