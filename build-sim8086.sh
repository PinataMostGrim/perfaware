# Build script for cli_sim8086.
# IMPORTANT: Run from project's root folder.

# Note: Configure these variables
SOURCES="../src/cli_sim8086.cpp"
INCLUDES="-I ../../common/src"

BUILD_FOLDER="sim8086/build"
OUT_EXE="sim8086"

# Note: This line is no longer necessary but I'm leaving it here for reference
# on how to do this.
# Save the script's folder
# SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

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
    COMPILER_FLAGS="-g -DSIM8086_SLOW=1 -Wno-null-dereference"
else
    # Making release build
    COMPILER_FLAGS="-DSIM8086_SLOW=0"
fi

# Create build folder if it doesn't exist
mkdir -p "$BUILD_FOLDER"

# Change to the build folder (and redirect stdout to /dev/null and the redirect stderr to stdout)
pushd $BUILD_FOLDER > /dev/null 2>&1

# Compile sim8086
clang $COMPILER_FLAGS $INCLUDES $SOURCES -o $OUT_EXE
popd > /dev/null 2>&1
