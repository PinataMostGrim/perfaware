# Build script for reference_haversine_main.c

# Note: Uncomment to debug commands
# set -ex

# Save the script's folder in order to construct full paths for each source.
# Some compilers seem to only output full paths on errors if this is done.
SCRIPT_FOLDER=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Note: Configure these variables
SRC_FOLDER="src"
BUILD_FOLDER="bin"
OUT_EXE="reference_haversine_main"

INCLUDES="-I../../common/src/ -I../../haversine/src/"
SOURCES="$SCRIPT_FOLDER/$SRC_FOLDER/reference_haversine_main.c"
LINKER_FLAGS="-lm"

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
    COMPILER_FLAGS="-g -O0 -Wall -Wno-unused-function -Wno-null-dereference -pedantic "
    # Uncomment to make build type explicit. May interfere with debuggers.
    # OUT_EXE="${OUT_EXE}_debug"
else
    # Making release build
    COMPILER_FLAGS=""
    # Uncomment to make build type explicit.
    # OUT_EXE="${OUT_EXE}_rel"
fi

# Create build folder if it doesn't exist
mkdir -p "$SCRIPT_FOLDER/$BUILD_FOLDER"

# Change to the build folder (and redirect stdout to /dev/null and the redirect stderr to stdout)
pushd "$SCRIPT_FOLDER/$BUILD_FOLDER" > /dev/null 2>&1

# Compile reference_haversine_main.c
gcc $COMPILER_FLAGS $INCLUDES $SOURCES -o $OUT_EXE $LINKER_FLAGS
popd > /dev/null 2>&1
