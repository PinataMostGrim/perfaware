/* TODO (Aaron):
*/

#include "base.h"
#include "base_string.h"
#include "base_memory.h"


// +------------------------------+
// Note (Aaron): C strings

global_function U64 GetStringLength(char *str)
{
    U64 count = 0;
    while(*str++) count++;

    return count;
}


global_function char *ArenaPushCString(memory_arena *arena, char *str)
{
    U64 strLength = GetStringLength(str);
    char *result = (char *)ArenaPushSize(arena, strLength);
    MemoryCopy(result, str, strLength);

    return result;
}


global_function char *ConcatStrings(char *stringA, char *stringB, memory_arena *arena)
{
    U64 sizeOfA = GetStringLength(stringA);
    U64 sizeOfB = GetStringLength(stringB);
    U64 resultSize = sizeOfA + sizeOfB;

    char *result = (char *)ArenaPushSizeZero(arena, resultSize + 1);
    MemoryCopy(result, stringA, sizeOfA);
    MemoryCopy(result + sizeOfA, stringB, sizeOfB);

    return result;
}


// +------------------------------+
// Note (Aaron): Length based strings

global_function Str8 String8(U8 *str, U64 size)
{
    Str8 result = { result.Str = str, result.Length = size };
    return result;
}


global_function Str8 Str8CString(U8 *cstr)
{
    U64 length = GetStringLength((char *)cstr);
    Str8 result = { result.Str = cstr, result.Length = length };
    return result;
}


global_function void Str8ListPushExplicit(Str8List *list, Str8 string, Str8Node *nodeMemory)
{
    nodeMemory->String = string;
    SLLQueuePush(list->First, list->Last, nodeMemory);
    list->NodeCount += 1;
    list->TotalSize += string.Length;
}


global_function void Str8ListPush(memory_arena *arena, Str8List *list, Str8 string)
{
    Str8Node *node = ArenaPushArray(arena, Str8Node, 1);
    Str8ListPushExplicit(list, string, node);
}


global_function Str8 Str8Pushfv(memory_arena *arena, char *fmt, va_list args)
{
    // va_list is stateful under some compilers. Duplicate in case we need to try a second time.
    va_list args2;
    va_copy(args2, args);

    // try to build the string in 1024 bytes
    U64 bufferSize = 1024;
    U8 *buffer = ArenaPushArray(arena, U8, bufferSize);
    U64 actualSize = vsnprintf((char *)buffer, bufferSize, fmt, args);

    Str8 result = {0};
    if (actualSize < bufferSize)
    {
        // if first try worked, put back what we didn't use and finish
        ArenaPopSize(arena, bufferSize - actualSize - 1);
        result = String8(buffer, actualSize);
    }
    else
    {
        // if first try failed, reset and try again with correct size
        ArenaPopSize(arena, bufferSize);
        U8 *fixedBuffer = ArenaPushArray(arena, U8, actualSize + 1);
        U64 finalSize = vsnprintf((char *)fixedBuffer, actualSize + 1, fmt, args2);
        result = String8(fixedBuffer, finalSize);
    }

    // end args2
    va_end(args2);

    return result;
}


global_function Str8 Str8Push(memory_arena *arena, Str8 string, B8 nullTerminate)
{
    U64 length = nullTerminate ? string.Length + 1 : string.Length;
    U8 *buffer = ArenaPushArray(arena, U8, length);
    MemoryCopy(buffer, string.Str, string.Length);
    if (nullTerminate)
    {
        MemoryZero(buffer + string.Length, 1);
    }

    Str8 result = String8(buffer, string.Length);
    return result;
}


global_function Str8 Str8Pushf(memory_arena *arena, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Str8 result = Str8Pushfv(arena, fmt,  args);
    va_end(args);

    return result;
}


global_function void Str8ListPushf(memory_arena *arena, Str8List *list, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Str8 string = Str8Pushfv(arena, fmt, args);
    va_end(args);

    Str8ListPush(arena, list, string);
}


global_function Str8 ConcatStr8(memory_arena *arena, Str8 strA, Str8 strB, B8 nullTerminate)
{
    memory_index totalSize = strA.Length + strB.Length + (nullTerminate ? 1 : 0);
    U8 *buffer = (U8 *)ArenaPushSize(arena, totalSize);
    MemoryCopy(buffer, strA.Str, strA.Length);
    MemoryCopy(buffer + strA.Length, strB.Str, strB.Length);
    if (nullTerminate)
    {
        MemoryZero(buffer + strA.Length + strB.Length, 1);
    }

    Str8 result = String8(buffer, totalSize);
    return result;
}
