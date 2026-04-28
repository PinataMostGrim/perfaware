#define DEBUG 0
#include "../common/src/daedalus.c"


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
        set_outfile("reference_haversine_verify");
        set_build_path_string(binPath);

#if DEBUG
        add_compiler_flag("-g");
        add_compiler_flag("-O0");
        add_compiler_flag("-Wall");
        add_compiler_flag("-Wno-unused-function");
        add_compiler_flag("-Wno-null-dereference");
        add_compiler_flag("-pedantic");
#endif

        string commonSrcInclude = b__memory_push_string_fmt(mem, "-I%s%s%s", rootPath.Str, PATH_SEP.Str, "../common/src");
        add_include_string(commonSrcInclude);

        string haversineSrcInclude = b__memory_push_string_fmt(mem, "-I%s%s%s", rootPath.Str, PATH_SEP.Str, "../haversine/src");
        add_include_string(haversineSrcInclude);

        string sourcePath = B__CONCAT_PATHS(mem, s2c(srcPath), "reference_haversine_verify.c");
        add_source_string(sourcePath);

        add_linker_flag("-lm");

        b__build();
    }
    else
    {
        result = 1;
    }

    DEBUG_EXPRESSION(fprintf(stdout, "[DEBUG] Memory used: %zu / %zu bytes (%.2f%%)\n", mem->Used, mem->Size, (((f32)mem->Used / (f32)mem->Size) * 100)));

    return result;
}
