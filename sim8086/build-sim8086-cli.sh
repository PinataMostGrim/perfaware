# Build script for cli_sim8086.

# Note: Uncomment to debug commands
# set -ex

# Note: Save the script's folder in order to construct full paths for each source.
# Some compilers seem to only output full paths on errors if this is done.
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Note: Configure these variables
SRC_FOLDER="src"
BUILD_FOLDER="bin"
OUT_EXE="sim8086"

INCLUDES="-I $SCRIPT_DIR/../common/src"
SOURCES="$SCRIPT_DIR/$SRC_FOLDER/sim8086_cli.cpp"

# Optionally set debug mode here:
DEBUG=1

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
    # Uncomment to make build type explicit. May interfere with debuggers.
    OUT_EXE="${OUT_EXE}_debug"
else
    # Making release build
    COMPILER_FLAGS="-DSIM8086_SLOW=0"
    # Uncomment to make build type explicit.
    # OUT_EXE="${OUT_EXE}_rel"
fi

# Create build folder if it doesn't exist
mkdir -p "$SCRIPT_DIR/$BUILD_FOLDER"

# Change to the build folder (and redirect stdout to /dev/null and the redirect stderr to stdout)
pushd $SCRIPT_DIR/$BUILD_FOLDER > /dev/null 2>&1

# Compile
g++ $COMPILER_FLAGS $INCLUDES $SOURCES -o $OUT_EXE
popd > /dev/null 2>&1
