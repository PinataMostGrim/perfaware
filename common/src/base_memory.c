/* TODO (Aaron):
    - Add run-time handling for when assertions would fail
        - What behaviour would we expect?
        - Return a null pointer?
        - Casey mentioned that he always returns a stub that can be used but is zeroed out every frame
*/

#if __linux__
#include <sys/mman.h>
#endif

#include "base.h"
#include "base_types.h"
#include "base_memory.h"


global_function void* MemoryReserve(size_t size)
{
#if __linux__
    void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED)
    {
        result = 0;
    }

    return result;

#elif _WIN32
    Assert(FALSE && "Platform not supported");

#endif

    Assert(FALSE && "Platform not supported");
    return 0;
}


global_function B32 MemoryCommit(void *base, size_t size)
{
#if __linux__
    mprotect(base, size, PROT_READ | PROT_WRITE);
    return 1;

#elif __WIN32
    Assert(FALSE && "Platform not supported");

#endif

    Assert(FALSE && "Platform not supported");
    return 0;
}

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
