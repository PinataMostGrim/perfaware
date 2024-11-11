#include <stdint.h>
#include <sys/mman.h>

// #include "base_memory.h"
// #include "base_memory.c"

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

#define Kilobytes(Value) ((Value)*1024LL)


int main(int argc, char const *argv[])
{
    u64 size4K = Kilobytes(4);
    u64 size8K = Kilobytes(4*2);
    u8 *base = mmap(0, size8K, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // Note (Aaron): This will fail without mprotect() being called first.
    // *(u64*)base = 12;

    mprotect(base, size4K, PROT_READ | PROT_WRITE);

    // Note (Aaron): This will succeed because mprotect has changed the entire allocation to be read / write
    *(u64*)base = 12;

    // Note (Aaron): This will fail because mprotect hasn't changed the second page to be read / write
    *(u64*)(base + Kilobytes(4)) = 12;


    return 0;
}
