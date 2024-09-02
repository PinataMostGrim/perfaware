#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#if LINUX
#include <sys/mman.h>
#endif

#include "buffer.h"


static buffer BufferAllocate(size_t sizeBytes)
{
    buffer result = {0};

#if LINUX
    int protectionFlags = PROT_READ | PROT_WRITE;
    int mappingFlags = MAP_ANONYMOUS | MAP_PRIVATE;
    result.Data = (uint8_t *)mmap(NULL, sizeBytes, protectionFlags, mappingFlags, -1, 0);
    if (result.Data == MAP_FAILED)
    {
        return result;
    }
#else
    result.Data = (uint8_t *)malloc(sizeBytes);
    if (!result.Data)
    {
        return result;
    }
#endif

    result.SizeBytes = sizeBytes;
    return result;
}


static void BufferFree(buffer *buff)
{
    if (buff->Data)
    {
#if LINUX
        munmap(buff->Data, buff->SizeBytes);
#else
        free(buff->Data);
#endif
    }

    uint64_t size = sizeof(buffer);
    unsigned char *dest = (unsigned char *)buff;
    while(size--) *dest++ = (unsigned char)0;
}


static uint32_t BufferIsValid(buffer buff)
{
    uint32_t result = (buff.Data != 0);
    return result;
}
