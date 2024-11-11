#include <stdint.h>

#if __linux__
#include <sys/mman.h>
#endif

#if _WIN32
#include <windows.h>
#endif

#include "base_memory.h"
#include "base_arena.h"

#include "base_memory.c"
#include "base_arena.c"


typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t b32;


#define WINDOWS _WIN32
#define LINUX __linux__

#define UNUSED(x) (void)(x)

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ARENA_MAX Gigabytes(1)


int main(int argc, char const *argv[])
{
    UNUSED(Sign64);
    UNUSED(Sign32);


// Test the memory reserve and commit functions
#if 0
    u64 size4K = Kilobytes(4);
    u64 size8K = Kilobytes(4*2);

    u8 *base = MemoryReserve(size8K);

    // Note (Aaron): This will fail without MemoryCommit() being called first.
    // *(u64*)base = 12;

    MemoryCommit(base, size4K);

    // Note (Aaron): This will succeed because mprotect has changed the entire allocation to be read / write
    *(u64*)base = 12;

    // Note (Aaron): This will fail because mprotect hasn't changed the second page to be read / write
    *(u64*)(base + Kilobytes(4)) = 12;
#endif


// Test that ArenaAllocate() primary use-case works as expected
#if 0
    u64 size4K = Kilobytes(4);
    u64 size8K = Kilobytes(4*2);

    // memory_arena arena = ArenaAllocate(size4K, 0);
    memory_arena arena = ArenaAllocate(size4K, size8K);
    // memory_arena arena = ArenaAllocate(size4K, ARENA_MAX);

    // Note (Aaron): This will succeed because mprotect has changed the entire allocation to be read / write
    *arena.PositionPtr = 12;

    // Note (Aaron): This will fail because mprotect hasn't changed the second page to be read / write
    ArenaPushSize(&arena, Kilobytes(4));
    *arena.PositionPtr = 12;

#endif

// Test that ArenaPushSize() correctly hits an assert if we push past max size
#if 0
    u64 size = Kilobytes(4);
    memory_arena arena = ArenaAllocate(size, size);

    ArenaPushSize(&arena, size);
    ArenaPushSize(&arena, 1);

#endif

// Test that ArenaPushSize() correctly hits an assert if we push past max size
#if 0
    u64 size = Kilobytes(4);
    u64 maxSize = Kilobytes(16);
    memory_arena arena = ArenaAllocate(size, maxSize);

    U8 *ptr1 = ArenaPushSize(&arena, size);
    ArenaPushSize(&arena, size);

    *ptr1 = (U64)12;
    *arena.PositionPtr = (U64)12;

#endif


// Test that the arena grows up to a large value
#if 1
    u64 chunkSize = Kilobytes(4);
    memory_arena arena = ArenaAllocate(chunkSize, ARENA_MAX);

    U8 *ptr = 0;
    for (int i = 0; i < (ARENA_MAX / chunkSize) + 1; ++i)
    {
        if ((arena.Size + chunkSize) >= ARENA_MAX)
        {
            int bp = 0;
        }

        ptr = ArenaPushSize(&arena, chunkSize);

        // Note (Aaron): Write into each page so that it is committed for real.
        *ptr = (U64)12;
    }

#endif

    return 0;
}
