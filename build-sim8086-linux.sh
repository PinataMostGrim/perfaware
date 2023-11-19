# Build script for sim8086-linux.
# IMPORTANT: Run from project's root folder.

# Note: Save the script's folder in order to construct full paths for each source.
# Some compilers seem to only output full paths on errors if this is done.
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )


# Note: Configure these variables
BUILD_FOLDER="sim8086/bin"
SRC_FOLDER="sim8086/src"
OUT_EXE="sim8086-linux"
OUT_LIB="sim8086_application.so"

INCLUDES="-I$SCRIPT_DIR/common/src -I$SCRIPT_DIR/$SRC_FOLDER/imgui -I$SCRIPT_DIR/$SRC_FOLDER/imgui/backends"
SOURCES="$SCRIPT_DIR/$SRC_FOLDER/sim8086_linux.cpp"
LIB_SOURCES="$SCRIPT_DIR/$SRC_FOLDER/sim8086_application.cpp"
LINKER_FLAGS="-lGL -lglfw"


# 0 for disabled, 1 for enabled
DIAGNOSTICS=1

# Sets DEBUG environment variable to 0 if
# it isn't already defined
if [ -z $DEBUG ]
then
    DEBUG=0
fi

# Set DEBUG environment variable to 1 for debug builds
if [ $DEBUG = "1" ]
then
    # Making debug build
    COMPILER_FLAGS="-g -Wno-null-dereference -DSIM8086_DIAGNOSTICS=$DIAGNOSTICS"
else
    # Making release build
    COMPILER_FLAGS="-DSIM8086_DIAGNOSTICS=$DIAGNOSTICS"
fi

# Create build folder if it doesn't exist
mkdir -p "$SCRIPT_DIR/$BUILD_FOLDER"

# Change to the build folder (and redirect stdout to /dev/null and the redirect stderr to stdout)
pushd $SCRIPT_DIR/$BUILD_FOLDER > /dev/null 2>&1

# Compile
echo WAITING FOR APPLICATION CODE TO COMPILE > sim8086_lock.tmp
g++ -fPIC $COMPILER_FLAGS $INCLUDES $LIB_SOURCES --shared -o $OUT_LIB
rm sim8086_lock.tmp
g++ $COMPILER_FLAGS $INCLUDES $SOURCES -o $OUT_EXE $LINKER_FLAGS

popd > /dev/null 2>&1
