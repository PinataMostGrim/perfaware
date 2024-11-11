#include <stdint.h>
#include <sys/mman.h>

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


b32 UnCommitMemory()
{
    return 1;
}


int main(int argc, char const *argv[])
{
    UNUSED(Sign64);
    UNUSED(Sign32);

// Note (Aaron): Test the OS reserve and commit functions
#if 0
    u64 size4K = Kilobytes(4);
    u64 size8K = Kilobytes(4*2);

    u8 *base = MemoryReserve(size8K);

    // Note (Aaron): This will fail without mprotect() being called first.
    // *(u64*)base = 12;

    MemoryCommit(base, size4K);

    // Note (Aaron): This will succeed because mprotect has changed the entire allocation to be read / write
    *(u64*)base = 12;

    // Note (Aaron): This will fail because mprotect hasn't changed the second page to be read / write
    *(u64*)(base + Kilobytes(4)) = 12;
#endif


// Note (Aaron): Test that ArenaAllocate()
#if 1
    u64 size4K = Kilobytes(4);
    u64 size8K = Kilobytes(4*2);

    // memory_arena arena = ArenaAllocate(size4K, size8K);
    // memory_arena arena = ArenaAllocate(size4K, 0);
    memory_arena arena = ArenaAllocate(size4K, ARENA_MAX);

    // Note (Aaron): This will succeed because mprotect has changed the entire allocation to be read / write
    *arena.PositionPtr = 12;

    // Note (Aaron): This will fail because mprotect hasn't changed the second page to be read / write
    ArenaPushSize(&arena, Kilobytes(4));
    *arena.PositionPtr = 12;

#endif

    return 0;
}
