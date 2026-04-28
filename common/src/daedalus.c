/* Notes:
 - Requires C99 for variadic arguments in macros
 - Strings are considered immutable once created
 - Strings are null terminated so as to be interchangeable with c strings and can be used with printf format functions
 - Insert the REBUILD(compiler) macro at the start of main() to automatically rebuild the build program if it changes
 - Add "#define DEBUG 1" before importing this file to enable the "DEBUG_EXPRESSION(expression)" macro and for additional diagnostics information


Example build program:

#define DEBUG 1
#include "daedalus.c"


int main(int argc, char const *argv[])
{
    REBUILD("clang");

    int result = 0;

    size_t memorySizeBytes = Megabytes(1);
    memory *mem = b__initialize(memorySizeBytes);
    if (b__memory_is_valid(mem))
    {
        string rootPath = get_dir_path_abs_argv0(mem);
        string srcPath = B__CONCAT_PATHS(mem, s2c(rootPath), "src");
        string binPath = B__CONCAT_PATHS(mem, s2c(rootPath), "bin");

        set_compiler("clang");
        set_outfile("hello");
        set_build_path_string(binPath);

        add_compiler_flag("-O0");

        string commonSrcInclude = b__memory_push_string_fmt(mem, "-I%s%s%s", srcPath.Str, PATH_SEP.Str, "../common");
        add_include_string(commonSrcInclude);

        string helloPath = B__CONCAT_PATHS(mem, s2c(srcPath), "hello.c");
        add_source_string(helloPath);

        b__build();
    }
    else
    {
        result = 1;
    }

    DEBUG_EXPRESSION(fprintf(stdout, "[DEBUG] Memory used: %zu / %zu bytes (%.2f%%)\n", mem->Used, mem->Size, (((f32)mem->Used / (f32)mem->Size) * 100)));

    return result;
}

*/

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#if __WIN32
// TODO (Aaron): Last modified time needs to be implemented for Windows
#else
#include <sys/stat.h>
#include <time.h>

#include <unistd.h>
#include <limits.h>
#endif

#if DEBUG
#define DEBUG_EXPRESSION(expression) expression
#else
#define DEBUG_EXPRESSION(expression) /* do nothing */
#endif

#define b__true 1
#define b__false 0

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

// Note (Aaron): Macro to return the folder the program resides in. Must be used in main().
#define get_dir_path_abs_argv0(arena) b__get_dir_path_abs(arena, (char *)argv[0])

// Note (Aaron): Add this macro to the start of the main function to rebuild the build application
// on execution if anything in it has changed. Requires the out file the be the same name  as the
// source file, and be placed in the same folder.
#define REBUILD(compiler) b__rebuild(compiler, __FILE__)

// Note (Aaron): Macros to automatically append a NULL terminating parameter to sentinel methods
#define B__CONCAT(memory, first, ...) b__concat_sentinel(memory, first, __VA_ARGS__, NULL);
#define B__CONCAT_PATHS(memory, ...) b__concat_paths_sentinel((memory), __VA_ARGS__, NULL)

// Note (Aaron): Macro for string to char * conversion and vice versa
#define s2c(string) (char *)(string.Str)
#define c2s(string) b__string_from_c_string(string)


typedef int8_t u8;
typedef int64_t u64;
typedef int32_t b32;
typedef float f32;

typedef struct memory memory;
struct memory
{
    u8 *BasePtr;
    u8 *PositionPtr;
    size_t Size;
    size_t Used;
};

typedef struct string string;
struct string
{
    u8 *Str;
    u64 Length;
};

typedef struct string_builder_node string_builder_node;
struct string_builder_node
{
    string_builder_node *Next;
    string String;
};

typedef struct string_builder string_builder;
struct string_builder
{
    string_builder_node *First;
    string_builder_node *Last;
    u64 NodeCount;
    u64 TotalSize;
    memory *Arena;
};

#if _WIN32
static char PATH_SEP_CHAR = '\\';
#else
static char PATH_SEP_CHAR = '/';
#endif

static string PATH_SEP;
static string COMPILER;
static string OUT_FILE;
static string BUILD_PATH;
static string_builder SOURCES;
static string_builder COMPILER_FLAGS;
static string_builder INCLUDES;
static string_builder LINKER_FLAGS;

static memory MEMORY;

static b32 INITIALIZED = b__false;


// Call this to initialize memory and string builders
memory *b__initialize(size_t memorySizeBytes);


// String manipulation declarations
string b__string(u8 *str, u64 size);
string b__string_from_c_string(char *str);
string b__memory_push_string_fmt(memory *arena, char *fmt, ...) __attribute__((format (printf, 2, 3)));
string b__memory_push_string_fmt_va(memory *arena, char *fmt, va_list args);
b32 b__string_is_valid(string str);

string_builder b__string_builder(memory *arena);
b32 b__string_builder_push(string_builder *builder, string str);
b32 b__string_builder_push_fmt(string_builder *builder, char *fmt, ...) __attribute__((format (printf, 2, 3)));
char *b__string_builder_to_c_string(string_builder *builder);
char *_b__string_builder_to_c_string(memory *arena, string_builder *builder);
string b__string_builder_to_string(string_builder *builder);

u64 b__strlen(char *str);
int b__find_last_char(const char *str, char c);
int b__find_last_char_string(string str, char c);


// Build-related declarations
string b__get_path_abs(memory *arena, char *path);
string b__get_dir_path_abs(memory *arena, char *path);

void set_build_path(char *fmt, ...) __attribute__((format (printf, 1, 2)));
void set_build_path_string(string buildPath);

void add_compiler_flag(char *fmt, ...) __attribute__((format (printf, 1, 2)));
void add_compiler_flag_string(string flag);

void add_include(char *fmt, ...) __attribute__((format (printf, 1, 2)));
void add_include_string(string flag);

void add_source(char *fmt, ...) __attribute__((format (printf, 1, 2)));
void add_source_string(string source);

void add_linker_flag(char *fmt, ...) __attribute__((format (printf, 1, 2)));
void add_linker_flag_string(string str);


// Utility declarations
void b__write_c_string(char *str);
void b__write_string(string str);
int b__execute_command(char *command);


// Memory-related declarations
b32 b__memory_is_valid(memory *arena);
void *b__memory_copy(void *destPtr, void *sourcePtr, size_t size);
b32 b__memory_set(void *destPtr, int c, size_t count);

memory b__memory_allocate(size_t size);
#define b__memory_push_array(arena, type, count) (type *)b__memory_push((arena), sizeof(type)*(count))


// Internal declarations
string _b__memory_push_string_fmt_va(memory *arena, b32 nullTerminate, char *fmt, va_list args);
string _b__string_builder_to_string(memory *arena, string_builder *builder, b32 nullTerminate);
b32 _b__string_builder_push(memory *arena, string_builder *builder, string str);
b32 _b__string_builder_push_fmt(memory *arena, string_builder *builder, char *fmt, ...) __attribute__((format (printf, 3, 4)));



size_t b__get_file_modified_time_seconds(char *filePath)
{
    size_t result = 0;

#if _WIN32

    fprintf(stderr, "[ERROR] Not implemented for Windows\n");

#else

    // Linux
    struct stat sb;
    int statResult = stat(filePath, &sb);
    if (statResult == 0)
    {
        struct timespec mod_time = sb.st_mtim;
        result = mod_time.tv_sec;
    }
    else
    {
        perror("[ERROR] stat");
    }

#endif

    return result;
}


b32 b__rebuild(char *compiler, char *file)
{
    b32 result = b__false;

    DEBUG_EXPRESSION(fprintf(stdout, "[DEBUG] compiler: %s\n", compiler));
    DEBUG_EXPRESSION(fprintf(stdout, "[DEBUG] file: %s\n", file));

    // Get the out file name
    int fileLength = b__strlen(file);
    int outFileLength = b__find_last_char(file, '.');

    DEBUG_EXPRESSION(fprintf(stdout, "[DEBUG] fileLength: %i\n", fileLength);)
    DEBUG_EXPRESSION(fprintf(stdout, "[DEBUG] outFileLength: %i\n", outFileLength);)

    char outfileBuffer[outFileLength + 1];
    char *outfile = b__memory_copy(outfileBuffer, file, outFileLength);
    b__memory_set((outfileBuffer + outFileLength), 0, 1);

    // Ensure the out file path has less characters than the source file
    if (outFileLength > 0 && outFileLength < fileLength)
    {
        DEBUG_EXPRESSION(fprintf(stdout, "[DEBUG] Out file name was parseable\n");)

        size_t sourceLastModified = b__get_file_modified_time_seconds(file);
        size_t outfileLastModified = b__get_file_modified_time_seconds(outfile);

        // Note (Aaron): Forces the out file modification to be earlier than the source
        // modification. Useful for debugging but will cause an infinite loop if actually building.
        // DEBUG_EXPRESSION(outfileLastModified = 1);

        // If we were able to determine when source and out file were last modified
        if (sourceLastModified > 0 && outfileLastModified > 0)
        {
            if (sourceLastModified > outfileLastModified)
            {
                DEBUG_EXPRESSION(fprintf(stdout, "[DEBUG] Source file was modified most recently\n"));

                fprintf(stdout, "[INFO] Detected build file '%s' has been modified since last build\n", file);

                // Determine build command length
                // TODO: Will need to handle file execution differences between windows and linux (./ vs .exe)
                char *buildFmt = "%s %s -o %s && %s";  // compiler, file, outfile, outfile
                int commandLength = snprintf(0, 0, buildFmt, compiler, file, outfile, outfile);
                DEBUG_EXPRESSION(fprintf(stdout, "[DEBUG] Write Length: %i\n", commandLength));

                // Note (Aaron): +1 for the null terminator
                char buildCommand[commandLength + 1];

                int writeLength = snprintf(buildCommand, commandLength + 1, buildFmt, compiler, file, outfile, outfile);

                if (commandLength == writeLength)
                {
                    DEBUG_EXPRESSION(fprintf(stdout, "[DEBUG] Build command fits in buffer\n"));
                    DEBUG_EXPRESSION(fprintf(stdout, "[DEBUG] %s\n", buildCommand));

                    fprintf(stdout, "[INFO] Rebuilding '%s':\n", outfile);
                    fprintf(stdout, "[INFO] Executing command: %s\n", buildCommand);

                    b__execute_command(buildCommand);
                    exit(1);
                }
                else
                {
                    fprintf(stderr, "[ERROR] Build command string does not fit in its buffer\n");
                }
            }
            else
            {
                // Note (Aaron): Nothing to do. Recompile the program as normal.
                DEBUG_EXPRESSION(fprintf(stdout, "[DEBUG] Out file was modified most recently. No need to rebuild.\n");)
            }
        }
        else
        {
            fprintf(stderr, "[ERROR] Unable to determine last modified date of source or out file\n");
        }
    }
    else
    {
        fprintf(stderr, "[ERROR] Out file path length is the same as the source file path length. Unable to determine if a rebuild is necessary.\n");
    }

    return result;
}


memory *b__initialize(size_t memorySizeBytes)
{
    if (!INITIALIZED)
    {
        MEMORY = b__memory_allocate(memorySizeBytes);
        if (b__memory_is_valid(&MEMORY))
        {
            PATH_SEP = b__string_from_c_string(&PATH_SEP_CHAR);

            SOURCES = b__string_builder(&MEMORY);
            COMPILER_FLAGS = b__string_builder(&MEMORY);
            INCLUDES = b__string_builder(&MEMORY);
            LINKER_FLAGS = b__string_builder(&MEMORY);

            INITIALIZED = b__true;
        }
    }

    return &MEMORY;
}


memory b__memory_allocate(size_t size)
{
    memory result = {0};

    u8 *base = malloc(size);
    if (base)
    {
        result.BasePtr = base;
        result.PositionPtr = base;
        result.Size = size;
        result.Used = 0;
    }

    return result;
}


b32 b__memory_is_valid(memory *arena)
{
    b32 result = (arena->BasePtr != 0);
    return result;
}


b32 b__memory_set(void *destPtr, int c, size_t count)
{
    b32 result = b__false;
    if (count > 0)
    {
        unsigned char *dest = (unsigned char *)destPtr;
        while(count--) *dest++ = (unsigned char)c;

        result = b__true;
    }

    return result;
}


void *b__memory_copy(void *destPtr, void *sourcePtr, size_t size)
{
    unsigned char *source = (unsigned char *)sourcePtr;
    unsigned char *dest = (unsigned char *)destPtr;
    while(size--) *dest++ = *source++;

    return destPtr;
}


void *b__memory_push(memory *arena, size_t size)
{
    void *result = 0;
    if (size != 0)
    {
        if ((arena->Used + size) <= arena->Size)
        {
            result = arena->BasePtr + arena->Used;
            arena->Used += size;
            arena->PositionPtr = arena->BasePtr + arena->Used;
        }
        else
        {
            DEBUG_EXPRESSION(fprintf(stderr, "[ERROR] Unable to push %zu bytes into arena, only %zu bytes free\n", size, (arena->Size - arena->Used)));
        }
    }
    else
    {
        DEBUG_EXPRESSION(fprintf(stdout, "[WARNING] Attempted to push 0 bytes into arena\n"));
    }

    return result;
}


void *b__memory_pop(memory *arena, size_t size)
{
    if (arena->Used < size)
    {
        DEBUG_EXPRESSION(fprintf(stderr, "[ERROR] Attempted to free more space than has been filled (currently used: %zu bytes,requested free: %zu)\n", arena->Used, size));

        size = arena->Used;
    }

    arena->Used -= size;
    arena->PositionPtr = arena->BasePtr + arena->Used;
    void *result = arena->BasePtr + arena->Used;
    return result;
}


char *b__memory_push_char(memory *arena, char c, size_t count)
{
    char *result = (char *)b__memory_push(arena, sizeof(char) * count);
    if (result)
    {
        b__memory_set(result, c, sizeof(char) * count);
    }

    return result;
}


string b__string(u8 *str, u64 length)
{
    string result = { result.Str = str, result.Length = length };

    DEBUG_EXPRESSION(if (*(str + length) != 0) fprintf(stdout, "[WARNING] String '%s' does not null terminate. This will cause problems if used with printf format functions.\n", str));

    return result;
}


string b__string_from_c_string(char *str)
{
    u64 length = b__strlen(str);
    string result = b__string((u8 *)str, length);

    return result;
}


// Note (Aaron): By convention, all strings should null terminate unless special circumstances
// apply, such as doing concatenate operations.
string _b__memory_push_string(memory *arena, string str, b32 nullTerminate)
{
    string result = {0};

    // Note (Aaron): Add +1 byte for the null terminating character
    size_t size = nullTerminate ? str.Length + 1 : str.Length;

    u8 *ptr = b__memory_push(arena, size);
    if (ptr)
    {
        b__memory_copy(ptr, str.Str, str.Length);
        result.Str = ptr;
        result.Length = str.Length;

        if (nullTerminate)
        {
            // Add null terminating character
            b__memory_set((ptr + str.Length), 0, 1);
        }
    }

    return result;
}


string b__memory_push_string(memory *arena, string str)
{
    return _b__memory_push_string(arena, str, b__true);
}


string b__memory_push_string_fmt(memory *arena, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    string result = b__memory_push_string_fmt_va(arena, fmt, args);
    va_end(args);

    return result;
}


string b__memory_push_string_fmt_va(memory *arena, char *fmt, va_list args)
{
    return _b__memory_push_string_fmt_va(arena, b__true, fmt, args);
}


string _b__memory_push_string_fmt_va(memory *arena, b32 nullTerminate, char *fmt, va_list args)
{
    string result = {0};

    // Note (Aaron): va_list is stateful under some compilers. Duplicate it in case we need to
    // try a second time.
    va_list args2;
    va_copy(args2, args);

    u64 bufferSize = 512;
    u8 *str = b__memory_push_array(arena, u8, bufferSize);
    if (str)
    {
        // Note (Aaron): vsnprintf adds the null terminating character at the end but returns
        // only the size of the c string.
        u64 actualSize = vsnprintf((char *)str, bufferSize, fmt, args);
        if (actualSize < bufferSize)
        {
            // Note (Aaron): If the first try worked, put back what we didn't use and finish
            size_t popSize = nullTerminate ? bufferSize - actualSize - 1 : bufferSize - actualSize;
            b__memory_pop(arena, popSize);

            result = b__string(str, actualSize);
        }
        else
        {
            // Note (Aaron): If first try failed, reset and try again with the correct size
            b__memory_pop(arena, bufferSize);
            str = b__memory_push_array(arena, u8, actualSize + 1);
            if (str)
            {
                // Note (Aaron): vsnprintf places the null terminator at the end of the length given to it
                // so we need to specify an extra byte for the null terminator regardless of whether we use
                // it or not.
                u64 finalSize = vsnprintf((char *)str, actualSize + 1, fmt, args2);
                result = b__string(str, finalSize);

                if (!nullTerminate)
                {
                    b__memory_pop(arena, 1);
                }
            }
            else
            {
                fprintf(stderr, "[ERROR] Unable to push string '%s' into memory\n", fmt);
            }
        }
    }
    else
    {
        fprintf(stderr, "[ERROR] Unable to push string '%s' into memory\n", fmt);
    }

    va_end(args2);

    return result;
}


b32 b__string_is_valid(string str)
{
    b32 result = str.Str && (str.Length > 0);
    return result;
}


string_builder b__string_builder(memory *arena)
{
    string_builder result = {0};
    result.Arena = arena;

    return result;
}


b32 b__string_builder_push(string_builder *builder, string str)
{
    b32 result = _b__string_builder_push(builder->Arena, builder, str);
    return result;
}


b32 _b__string_builder_push(memory *arena, string_builder *builder, string str)
{
    b32 result = b__false;

    if (b__string_is_valid(str))
    {
        string_builder_node *node = b__memory_push_array(arena, string_builder_node, 1);
        if (node)
        {
            node->String = str;
            node->Next = 0;

            if (builder->First == 0)
            {
                builder->First = node;
                builder->Last = node;
            }
            else
            {
                builder->Last->Next = node;
                builder->Last = node;
            }

            builder->NodeCount++;
            builder->TotalSize += str.Length;

            result = b__true;
        }
        else
        {
            DEBUG_EXPRESSION(fprintf(stderr, "[ERROR] Unable to push string builder node into builder\n"));
        }
    }
    else
    {
        fprintf(stderr, "[ERROR] Unable to push invalid string into builder\n");
    }

    return result;
}


b32 b__string_builder_push_fmt(string_builder *builder, char *fmt, ...)
{
    b32 result = b__false;

    va_list args;
    va_start(args, fmt);

    string str = _b__memory_push_string_fmt_va(builder->Arena, b__false, fmt, args);
    if (b__string_is_valid(str))
    {
        result = b__string_builder_push(builder, str);
    }

    va_end(args);

    return result;
}


b32 _b__string_builder_push_fmt(memory *arena, string_builder *builder, char *fmt, ...)
{
    b32 result = b__false;

    va_list args;
    va_start(args, fmt);

    string str = _b__memory_push_string_fmt_va(arena, b__false, fmt, args);
    if (b__string_is_valid(str))
    {
        result = _b__string_builder_push(arena, builder, str);
    }

    va_end(args);

    return result;
}


char *b__string_builder_to_c_string(string_builder *builder)
{
    return _b__string_builder_to_c_string(builder->Arena, builder);
}


char *_b__string_builder_to_c_string(memory *arena, string_builder *builder)
{
    char * result = 0;
    string str = _b__string_builder_to_string(arena, builder, b__true);
    if (b__string_is_valid(str))
    {
        result  = (char *)str.Str;
    }

    return result;
}


string b__string_builder_to_string(string_builder *builder)
{
    return _b__string_builder_to_string(builder->Arena, builder, b__true);
}


string _b__string_builder_to_string(memory *arena, string_builder *builder, b32 nullTerminate)
{
    string result = {0};

    u64 totalLength = 0;
    u64 nodeCount = 0;

    u8 *start = arena->PositionPtr;
    string_builder_node *nextNode = builder->First;

    while (nextNode)
    {
        string current = _b__memory_push_string(arena, nextNode->String, b__false);
        if (b__string_is_valid(current))
        {
            totalLength += nextNode->String.Length;
            nextNode = nextNode->Next;
            nodeCount++;
        }
        else
        {
            fprintf(stderr, "[ERROR] Unable to push string '%s' into memory\n", nextNode->String.Str);
            nextNode = 0;
        }
    }

    if (nodeCount == builder->NodeCount)
    {
        if (nullTerminate)
        {
            // Add null terminating character
            u8 *ptr = (u8*)b__memory_push_char(arena, 0, 1);
            if (ptr)
            {
                result = b__string(start, totalLength);
            }
            else
            {
                fprintf(stderr, "[ERROR] String builder unable to append null terminator.\n");
            }
        }
        else
        {
            // Note (Aaron): We don't use the string constructor because we
            // do not want the null termination warning.
            result.Str = start;
            result.Length = totalLength;
        }
    }
    else
    {
        fprintf(stderr, "[ERROR] String builder node count mismatch. Expected '%ld' but received '%ld'\n", builder->NodeCount, nodeCount);
    }

    return result;
}


// Note (Aaron): Expects a null parameter as the final argument to mark the end of the
// variadic args.
string b__concat_sentinel(memory *arena, char *first, ...)
{
    string result = {0};

    va_list args;
    va_start(args, first);

    u8 *start = arena->PositionPtr;
    b32 success = b__true;

    char *str = first;
    while (str != NULL)
    {
        string current = _b__memory_push_string(arena, b__string_from_c_string(str), b__false);
        if (!b__string_is_valid(current))
        {
            success = b__false;
        }

        str = va_arg(args, char *);
    }

    // Note (Aaron): It is important to calculate 'end' before
    // the null terminator is pushed.
    u8 *end = arena->PositionPtr;

    // Note (Aaron): Append null terminator
    char *ptr = b__memory_push_char(arena, 0, 1);
    if (!ptr)
    {
        success = b__false;
    }

    if (success)
    {
        u64 length = (u64)(end - start);
        result = b__string(start, length);
    }

    return result;
}


// Note (Aaron): Expects a null parameter as the final argument to mark the end of the
// variadic args.
string b__concat_paths_sentinel(memory *arena, char *first, ...)
{
    // TODO (Aaron): Consider some extra checking to ensure we don't double up
    // path separators between elements

    string result = {0};

    va_list args;
    va_start(args, first);

    u8 *start = arena->PositionPtr;
    b32 success = b__true;

    char *str = first;
    while (str != NULL)
    {
        if (str != first)
        {
            string current = _b__memory_push_string(arena, PATH_SEP, b__false);
            if (!b__string_is_valid(current))
            {
                success = b__false;
            }
        }

        string current = _b__memory_push_string(arena, b__string_from_c_string(str), b__false);
        if (!b__string_is_valid(current))
        {
            success = b__false;
        }

        str = va_arg(args, char*);
    }

    // Note (Aaron): It is important to calculate 'end' before
    // the null terminator is pushed.
    u8 *end = arena->PositionPtr;

    // Note (Aaron): Append null terminator
    char *ptr = b__memory_push_char(arena, 0, 1);
    if (!ptr)
    {
        success = b__false;
    }

    if (success)
    {
        u64 length = (u64)(end - start);
        result = b__string(start, length);
    }

    return result;
}


u64 b__strlen(char *str)
{
    u64 lenth = 0;
    while(*str++) lenth++;

    return lenth;
}


// Note (Aaron): Returns the index of the last instance of 'c' in the provided
// string, or -1 if it is not present.
int b__find_last_char(const char *str, char c)
{
    int result = -1;
    for (int i = 0; str[i] != 0; ++i)
    {
        if (str[i] == c)
        {
            result = i;
        }
    }

    return result;
}


int b__find_last_char_string(string str, char c)
{
    int result = -1;
    for (int i = 0; i < str.Length; ++i)
    {
        if (*(str.Str + i) == c)
        {
            result = i;
        }
    }

    return result;
}


string b__get_path_abs(memory *arena, char *path)
{
    string result = {0};

#if __WIN32

    printf(stderr, "[ERROR] b__get_path_abs() not implemented for Windows");

#else

    char buffer[PATH_MAX];
    char *absPath = realpath(path, buffer);

    DEBUG_EXPRESSION(printf("[DEBUG] b__get_path_abs: %s\n", absPath));

    // Note (Aaron): tempStr points to memory allocated on the stack
    // and will become invalid as soon as the function returns.
    string tempStr = b__string_from_c_string(absPath);
    result = b__memory_push_string(arena, tempStr);

#endif

    return result;
}


// Returns the absolute path of the directory parsed out of 'path'.
string b__get_dir_path_abs(memory *arena, char *path)
{
    string result = {0};

    string filePathAbs = b__get_path_abs(arena, (char *)path);
    if (b__string_is_valid(filePathAbs))
    {
        int dirPathLength = b__find_last_char_string(filePathAbs, PATH_SEP_CHAR);
        if (dirPathLength > 0)
        {
            // Note (Aaron): Add an extra byte for the null terminator
            // that b__get_path_abs() pushed into memory.
            size_t popSize = filePathAbs.Length - dirPathLength + 1;
            b__memory_pop(arena, popSize);

            // Note (Aaron): Apply the null terminator
            char *ptr = b__memory_push_char(arena, 0, 1);
            if (ptr)
            {
                result = b__string(
                    filePathAbs.Str,
                    dirPathLength);
            }
            else
            {
                fprintf(stdout, "[ERROR] Unable to apply a null terminator character to '%s'\n", path);
            }
        }
        else
        {
            fprintf(stdout, "[WARNING] Unable to parse absolute directory path, no path separator (%c) found in '%s'\n", PATH_SEP_CHAR, path);
        }
    }
    else
    {
        fprintf(stdout, "[WARNING] Unable to push path string '%s' into memory\n", path);
    }

    return result;
}


void b__write_c_string(char *str)
{
    int length = b__strlen(str);
    write(1, str, length);
}


void b__write_string(string str)
{
    write(1, str.Str, str.Length);
}


int b__execute_command(char *command)
{
    FILE *fp;
    char buffer[1024];

    b__write_c_string("[INFO] Executing command: ");
    b__write_c_string(command);

    fp = popen(command, "r");
    if (fp == NULL)
    {
        b__write_c_string("[ERROR] popen() failed\n");
        return 1;
    }

    DEBUG_EXPRESSION(b__write_c_string("[INFO] Reading command output:\n"));
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        b__write_c_string(buffer);
    }

    int status = pclose(fp);
    if (status == -1)
    {
        b__write_c_string("[ERROR] pclose() failed\n");
        return 1;
    }

    DEBUG_EXPRESSION(b__write_c_string("[INFO] Command completed successfully\n");)

    return 0;
}


void set_compiler(char *compiler)
{
    COMPILER = b__string_from_c_string(compiler);
}


void set_outfile(char *outFile)
{
    OUT_FILE = b__string_from_c_string(outFile);
}


void set_build_path(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    BUILD_PATH = b__memory_push_string_fmt_va(&MEMORY, fmt, args);
    va_end(args);
}


void set_build_path_string(string buildPath)
{
    BUILD_PATH = buildPath;
}


void add_compiler_flag(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    string flag = b__memory_push_string_fmt_va(&MEMORY, fmt, args);
    va_end(args);

    add_compiler_flag_string(flag);
}


void add_compiler_flag_string(string flag)
{
    // Note (Aaron): This isn't ideal adding 24 bytes for a string_builder_node
    // only to push a single space, but the alternative is allocating an
    // entirely new string with the space added.
    b__string_builder_push(&COMPILER_FLAGS, b__string_from_c_string(" "));
    b__string_builder_push(&COMPILER_FLAGS, flag);
}


void add_include(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    string include = b__memory_push_string_fmt_va(&MEMORY, fmt, args);
    va_end(args);

    add_include_string(include);
}


void add_include_string(string flag)
{
    b__string_builder_push(&INCLUDES, b__string_from_c_string(" "));
    b__string_builder_push(&INCLUDES, flag);
}


void add_source(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    string source = b__memory_push_string_fmt_va(&MEMORY, fmt, args);
    va_end(args);

    add_source_string(source);
}


void add_source_string(string source)
{
    b__string_builder_push(&SOURCES, b__string_from_c_string(" "));
    b__string_builder_push(&SOURCES, source);
}


void add_linker_flag(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    string flag = b__memory_push_string_fmt_va(&MEMORY, fmt, args);
    va_end(args);

    add_linker_flag_string(flag);
}


void add_linker_flag_string(string flag)
{
    b__string_builder_push(&LINKER_FLAGS, b__string_from_c_string(" "));
    b__string_builder_push(&LINKER_FLAGS, flag);
}


int b__build()
{
    int result = 0;

    if (INITIALIZED)
    {
        b__write_c_string("[INFO] Constructing build command...\n");

        string_builder buildCommand = b__string_builder(&MEMORY);
        b32 error = b__false;

        // Construct the build command
        char *command = 0;

        // Add build path
        if (BUILD_PATH.Length > 0)
        {
            if (b__string_builder_push_fmt(&buildCommand, "mkdir -p %s && cd %s && ", BUILD_PATH.Str, BUILD_PATH.Str))
            {
                // Success
            }
            else
            {
                fprintf(stderr, "[ERROR] Unable to allocate memory for build path string\n");
                error = b__true;
            }
        }

        // Add compiler
        if (b__string_is_valid(COMPILER))
        {
            if (b__string_builder_push(&buildCommand, COMPILER))
            {
                // Success
            }
            else
            {
                fprintf(stderr, "[ERROR] Unable to add compiler string to build command\n");
                error = b__true;
            }
        }
        else
        {
            fprintf(stderr, "[ERROR] Compiler has not been set\n");
            error = b__true;
        }

        // Add compiler flags
        if (COMPILER_FLAGS.NodeCount > 0)
        {
            string compilerFlags = b__string_builder_to_string(&COMPILER_FLAGS);
            if (b__string_is_valid(compilerFlags))
            {
                if (b__string_builder_push(&buildCommand, compilerFlags))
                {
                    // Success
                }
                else
                {
                    fprintf(stderr, "[ERROR] Unable to allocate memory for compiler flags string\n");
                    error = b__true;
                }
            }
            else
            {
                fprintf(stderr, "[ERROR] Unable to allocate memory for compiler flags string\n");
                error = b__true;
            }
        }

        // Add includes
        if (INCLUDES.NodeCount > 0)
        {
            string includes = b__string_builder_to_string(&INCLUDES);
            if(b__string_is_valid(includes))
            {
                if (b__string_builder_push(&buildCommand, includes))
                {
                    // Success
                }
                else
                {
                    fprintf(stderr, "[ERROR] Unable to allocate memory for includes string\n");
                    error = b__true;
                }
            }
            else
            {
                fprintf(stderr, "[ERROR] Unable to allocate memory for includes string\n");
                error = b__true;
            }
        }

        // Add sources
        if (SOURCES.NodeCount > 0)
        {
            string sources = b__string_builder_to_string(&SOURCES);
            if (b__string_is_valid(sources))
            {
                if (b__string_builder_push(&buildCommand, sources))
                {
                    // Success
                }
                else
                {
                    fprintf(stderr, "[ERROR] Unable to allocate memory for sources string\n");
                    error = b__true;
                }
            }
            else
            {
                fprintf(stderr, "[ERROR] Unable to allocate memory for sources string\n");
                error = b__true;
            }
        }
        else
        {
            fprintf(stderr, "[ERROR] No source files have been set\n");
            error = b__true;
        }

        // Add out file
        if (b__string_is_valid(OUT_FILE))
        {
            if (b__string_builder_push_fmt(&buildCommand, " -o %s", OUT_FILE.Str))
            {
                // Success
            }
            else
            {
                fprintf(stderr, "[ERROR] Unable to allocate memory for out file string\n");
                error = b__true;
            }
        }
        else
        {
            fprintf(stderr, "[ERROR] No out file has been set\n");
            error = b__true;
        }

        // Push linker flags
        if (LINKER_FLAGS.NodeCount > 0)
        {
            string linkerFlags = b__string_builder_to_string(&LINKER_FLAGS);
            if (b__string_is_valid(linkerFlags))
            {
                if (b__string_builder_push(&buildCommand, linkerFlags))
                {
                    // Success
                }
                else
                {
                    fprintf(stderr, "[ERROR] Unable to allocate memory for linker flags string\n");
                    error = b__true;
                }
            }
            else
            {
                fprintf(stderr, "[ERROR] Unable to allocate memory for linker flags string\n");
                error = b__true;
            }
        }

        // Add newline character for the complete string
        if (b__string_builder_push_fmt(&buildCommand, "\n"))
        {
            // Success
        }
        else
        {
            fprintf(stderr, "[ERROR] Unable to allocate memory for newline character\n");
            error = b__true;
        }

        if (!error)
        {
            command = b__string_builder_to_c_string(&buildCommand);
            if (command)
            {
                // Success
            }
            else
            {
                fprintf(stderr, "[ERROR] Unable to allocate memory for build command string\n");
                error = b__true;
            }
        }
        else
        {
            fprintf(stderr, "[ERROR] Encountered an error. Unable to construct the build command\n");
        }

        if (!error && command)
        {
            // b__write_c_string("[DRY RUN] ");
            // b__write_c_string(command);
            result = b__execute_command(command);

            // TODO (Aaron): Execute the built application
            //  - Probably want to check for an argument before doing this
            //  - Also probably want to fork the process to run as a child so we don't have to go through the b__execute_command() function
            // if (result == 0)
            // {
            //     // Note (Aaron): Execute the built application
            //     char *buildExe = b__memory_push_c_string_fmt(&MISC, b__true, "./%s\n", OUT_FILE);
            //     b__execute_command(buildExe);
            // }
        }
    }
    else
    {
        fprintf(stderr, "[ERROR] Not initialized. Call b__initialize() first!\n");
    }

    return result;
}
