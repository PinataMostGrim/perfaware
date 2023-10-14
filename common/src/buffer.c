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


static void BufferFree(buffer *buff)
{
    if (buff->Data)
    {
        free(buff->Data);
    }

    uint64_t size = sizeof(buffer);
    unsigned char *dest = (unsigned char *)buff;
    while(size--) *dest++ = (unsigned char)0;
}
