# Build script for test_cache_sets.c

# Note: Uncomment to debug commands
# set -ex

# Save the script's folder in order to construct full paths for each source.
# Some compilers seem to only output full paths on errors if this is done.
SCRIPT_FOLDER=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Note: Configure these variables
SRC_FOLDER="src"
BUILD_FOLDER="bin"
OUT_EXE="test_cache_sets"

INCLUDES="-I../../common/src/"
SOURCES="$SCRIPT_FOLDER/$SRC_FOLDER/test_cache_sets.c"
LINKER_FLAGS="-L. -l:cache_sets.a"

# Optionally set debug mode here:
DEBUG=1

# Set the DEBUG environment variable to 0 if it isn't already defined
if [ -z $DEBUG ]
then
    DEBUG=0
fi

if [ $DEBUG = "1" ]
then
    # Making debug build
    NASM_FLAGS="-f elf64 -g -F dwarf -O0 -w+all"
    COMPILER_FLAGS="-g -O0 -Wall -Wno-unused-function -Wno-null-dereference -pedantic "
else
    # Making release build
    NASM_FLAGS="-f elf64"
    COMPILER_FLAGS=""
fi

# Create build folder if it doesn't exist
mkdir -p "$SCRIPT_FOLDER/$BUILD_FOLDER"

# Change to the build folder (and redirect stdout to /dev/null and the redirect stderr to stdout)
pushd "$SCRIPT_FOLDER/$BUILD_FOLDER" > /dev/null 2>&1

# Package assembly into library using nasm
nasm  $NASM_FLAGS -o cache_sets.o $SCRIPT_FOLDER/$SRC_FOLDER/cache_sets.asm && \
ar -crs cache_sets.a cache_sets.o && \

# Compile test_cache_sets.c
gcc $COMPILER_FLAGS $INCLUDES $SOURCES -o $OUT_EXE $LINKER_FLAGS
popd > /dev/null 2>&1
