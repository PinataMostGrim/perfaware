/* TODO (Aaron):
    - Add run-time handling for when assertions would fail
        - What behaviour would we expect?
        - Return a null pointer?
        - Casey mentioned that he always returns a stub that can be used but is zeroed out every frame
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
