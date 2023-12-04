#!/usr/bin/env bash

# Build script for test_front_end.c

# Save the script's folder in order to construct full paths for each source.
# Some compilers seem to only output full paths on errors if this is done.
SCRIPT_FOLDER=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Note: Configure these variables
SRC_FOLDER="src"
BUILD_FOLDER="bin"
OUT_EXE="test_front_end"

INCLUDES=""
SOURCES="$SCRIPT_FOLDER/$SRC_FOLDER/test_front_end.c"
LINKER_FLAGS="-L. -l:nop_loop.a"

# Set the DEBUG environment variable to 0 if it isn't already defined
if [ -z $DEBUG ]
then
    DEBUG=0
fi

if [ $DEBUG = "1" ]
then
    # Making debug build
    COMPILER_FLAGS="-g -O0 -Wall -Wno-unused-function -Wno-null-dereference -pedantic"
else
    # Making release build
    COMPILER_FLAGS=""
fi

# Create build folder if it doesn't exist
mkdir -p "$SCRIPT_FOLDER/$BUILD_FOLDER"

# Change to the build folder (and redirect stdout to /dev/null and the redirect stderr to stdout)
pushd "$SCRIPT_FOLDER/$BUILD_FOLDER" > /dev/null 2>&1

# Package assembly into library using nasm
nasm -f elf64 -o nop_loop.o $SCRIPT_FOLDER/$SRC_FOLDER/nop_loop.asm && \
ar -crs nop_loop.a nop_loop.o && \

# Compile test_front_end.c
gcc $COMPILER_FLAGS $INCLUDES $SOURCES -o $OUT_EXE $LINKER_FLAGS
popd > /dev/null 2>&1
