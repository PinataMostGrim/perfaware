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


void* ReserveMemory(size_t size)
{
    void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED)
    {
        result = 0;
    }

    return result;
}


b32 CommitMemory(void *base, size_t size)
{
    mprotect(base, size, PROT_READ | PROT_WRITE);
    return 1;
}


b32 UnCommitMemory()
{
    return 1;
}


memory_arena ArenaAllocate(size_t size, size_t maxSize)
{
    memory_arena result = {0};

    // Note (Aaron): A value of 0 for maxSize indicates the arena is not growable.
    if (!maxSize)
    {
        maxSize = size;
    }

    // TODO (Aaron): Consider how to handle this in production code.
    Assert(maxSize >= size);

    u8 *base = ReserveMemory(maxSize);
    if (base)
    {
        CommitMemory(base, size);
        ArenaInitialize(&result, size, base);
    }

    return result;
}


int main(int argc, char const *argv[])
{
    UNUSED(Sign64);
    UNUSED(Sign32);

// Note (Aaron): Test the OS reserve and commit functions
#if 0
    u64 size4K = Kilobytes(4);
    u64 size8K = Kilobytes(4*2);

    u8 *base = ReserveMemory(size8K);

    // Note (Aaron): This will fail without mprotect() being called first.
    // *(u64*)base = 12;

    CommitMemory(base, size4K);

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
