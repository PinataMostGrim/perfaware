#include "base.h"
#include "base_string.h"


global_function String8 str8(U8 *str, U64 size)
{
    String8 result = { result.Str = str, result.Length = size };
    return result;
}


global_function String8 str8_cstring(U8 *cstr)
{
    U64 length = GetStringLength((char *)cstr);
    String8 result = { result.Str = cstr, result.Length = length };
    return result;
}


global_function void str8_list_push_explicit(String8List *list, String8 string, String8Node *nodeMemory)
{
    nodeMemory->String = string;
    SLLQueuePush(list->First, list->Last, nodeMemory);
    list->NodeCount += 1;
    list->TotalSize += string.Length;
}


global_function void str8_list_push(memory_arena *arena, String8List *list, String8 string)
{
    String8Node *node = ArenaPushArray(arena, String8Node, 1);
    str8_list_push_explicit(list, string, node);
}


global_function String8 str8_pushfv(memory_arena *arena, char *fmt, va_list args)
{
    // va_list is stateful under some compilers. Duplicate in case we need to try a second time.
    va_list args2;
    va_copy(args2, args);

    // try to build the string in 1024 bytes
    U64 bufferSize = 1024;
    U8 *buffer = ArenaPushArray(arena, U8, bufferSize);
    U64 actualSize = vsnprintf((char *)buffer, bufferSize, fmt, args);

    String8 result = {0};
    if (actualSize < bufferSize)
    {
        // if first try worked, put back what we didn't use and finish
        ArenaPopSize(arena, bufferSize - actualSize - 1);
        result = str8(buffer, actualSize);
    }
    else
    {
        // if first try failed, reset and try again with correct size
        ArenaPopSize(arena, bufferSize);
        U8 *fixedBuffer = ArenaPushArray(arena, U8, actualSize + 1);
        U64 finalSize = vsnprintf((char *)fixedBuffer, actualSize + 1, fmt, args2);
        result = str8(fixedBuffer, finalSize);
    }

    // end args2
    va_end(args2);

    return result;
}


global_function String8 str8_pushf(memory_arena *arena, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    String8 result = str8_pushfv(arena, fmt,  args);
    va_end(args);

    return result;
}


global_function void str8_list_pushf(memory_arena *arena, String8List *list, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    String8 string = str8_pushfv(arena, fmt, args);
    va_end(args);

    str8_list_push(arena, list, string);
}
