// Note (Aaron): Strings should be treated as immutable after construction.

#ifndef BASE_STRING_H
#define BASE_STRING_H

#include <stdarg.h>
#include <stdio.h>

#include "base_types.h"
#include "base_memory.h"


// +------------------------------+
// Note (Aaron): C strings
global_function U64 GetStringLength(char *str);
global_function char *ArenaPushCString(memory_arena *arena, char *str);


// +------------------------------+
// Note (Aaron): Length based strings

typedef struct
{
    U8 *Str;
    U64 Length;
} Str8;


typedef struct Str8Node Str8Node;
struct Str8Node
{
    Str8Node *Next;
    Str8 String;
};


typedef struct
{
    Str8Node *First;
    Str8Node *Last;
    U64 NodeCount;
    U64 TotalSize;
} Str8List;


global_function Str8 String8(U8 *str, U64 size);
// TODO (Aaron): Implement when I need it
// global_function Str8 Str8Range(U8 *first, U8 *opl);
global_function Str8 Str8CString(U8 *cstr);

#define STR8_LIT(s) String8((U8*)(s), sizeof(s) - 1)
#define STR8_EXPAND(s) (int)((s).Size), ((s).str)

// TODO (Aaron): Implement when I need it
// global_function Str8 Str8Prefix(Str8 str, U64 size);
// global_function Str8 Str8Chop(Str8 str, U64 amount);
// global_function Str8 Str8Postfix(Str8 str, U64 size);
// global_function Str8 Str8Skip(Str8 str, U64 amount);
// global_function Str8 Str8Substr(Str8 str, U64 first, U64 opl);

global_function void Str8ListPushExplicit(Str8List *list, Str8 string, Str8Node *nodeMemory);
global_function void Str8ListPush(memory_arena *arena, Str8List *list, Str8 string);
// TODO (Aaron): Implement when I need it
// global_function Str8 Str8Join(memory_arena *arena, Str8List *list, StringJoin *optionalJoin);
// global_function Str8List Str8Split(memory_arena *arena, Str8 string, U8 *split_characters, U32 count);

global_function Str8 Str8Pushfv(memory_arena *arena, char *fmt, va_list args);
global_function Str8 Str8Pushf(memory_arena *arena, char *fmt, ...);
global_function void Str8ListPushf(memory_arena *arena, Str8List *list, char *fmt, ...);

#endif // BASE_STRING_H
