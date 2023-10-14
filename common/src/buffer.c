#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "buffer.h"


static buffer BufferAllocate(size_t sizeBytes)
{
    buffer result = {0};

    result.Data = (uint8_t *)malloc(sizeBytes);
    if (!result.Data)
    {
        return result;
    }

    result.SizeBytes = sizeBytes;
    return result;
}


static void BufferFree(buffer *instance)
{
    if (instance->Data)
    {
        free(instance->Data);
    }

    uint64_t size = sizeof(buffer);
    unsigned char *dest = (unsigned char *)instance;
    while(size--) *dest++ = (unsigned char)0;
}
