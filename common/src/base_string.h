// Note (Aaron): Strings should be treated as immutable after construction.

#ifndef BASE_STRING_H
#define BASE_STRING_H

#include "memory_arena.h"
#include <stdarg.h>
#include <stdio.h>


typedef struct
{
    U8 *Str;
    U64 Length;
} String8;


typedef struct String8Node String8Node;
struct String8Node
{
    String8Node *Next;
    String8 String;
};


typedef struct
{
    String8Node *First;
    String8Node *Last;
    U64 NodeCount;
    U64 TotalSize;
} String8List;


global_function String8 str8(U8 *str, U64 size);
// TODO (Aaron): Implement when I need it
// global_function String8 str8_range(U8 *first, U8 *opl);
global_function String8 str8_cstring(U8 *cstr);

#define str8_lit(s) str8((U8*)(s), sizeof(s) - 1)

// TODO (Aaron): Implement when I need it
// global_function String8 str8_prefix(String8 str, U64 size);
// global_function String8 str8_chop(String8 str, U64 amount);
// global_function String8 str8_postfix(String8 str, U64 size);
// global_function String8 str8_skip(String8 str, U64 amount);
// global_function String8 str8_substr(String8 str, U64 first, U64 opl);

#define str8_expand(s) (int)((s).Size), ((s).str)


global_function void str8_list_push_explicit(String8List *list, String8 string, String8Node *nodeMemory);
global_function void str8_list_push(memory_arena *arena, String8List *list, String8 string);
// TODO (Aaron): Implement when I need it
// global_function String8 str8_join(memory_arena *arena, String8List *list, StringJoin *optionalJoin);
// global_function String8List str8_split(memory_arena *arena, String8 string, U8 *split_characters, U32 count);


global_function String8 str8_pushfv(memory_arena *arena, char *fmt, va_list args);
global_function String8 str8_pushf(memory_arena *arena, char *fmt, ...);
global_function void    str8_list_pushf(memory_arena *arena, String8List *list, char *fmt, ...);

#endif // BASE_STRING_H
