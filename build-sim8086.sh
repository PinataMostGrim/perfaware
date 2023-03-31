# Build script for sim8086.

# Save the script's folder
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Set the DEBUG environment variable to 0 if
# it isn't already defined
if [ -z $DEBUG ]
then
    DEBUG=0
fi

if [ $DEBUG = "1" ]
then
    # Making debug build
    CompilerFlags="-g -DSIM8086_SLOW=1 -Wno-null-dereference"
else
    # Making release build
    CompilerFlags="-DSIM8086_SLOW=0"
fi

BuildFolder="part01"

# Create build folder if it doesn't exist
mkdir -p "$SCRIPT_DIR/$BuildFolder"

# Change to the build folder (and redirect stdout to /dev/null and the redirect stderr to stdout)
pushd $BuildFolder > /dev/null 2>&1

# Compile sim8086
clang $CompilerFlags "$SCRIPT_DIR/part01/sim8086.cpp" -o "sim8086"
popd > /dev/null 2>&1
