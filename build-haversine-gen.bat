:: IMPORTANT: "vcvarsall.bat x64" must be executed in the shell first. Must be run from project root folder.

@echo off

:: NOTE: Set %DEBUG% to 1 for debug build
IF [%DEBUG%] == [1] (
    :: Making debug build
    set CompilerFlags=-nologo -Od -Gm- -MT -W4 -FC -wd4996 -wd4201 -DSIM8086_SLOW=1 -Zi -DEBUG:FULL
) ELSE (
    :: Making release build
    set CompilerFlags=-nologo -Od -Gm- -MT -W4 -FC -DSIM8086_SLOW=0
)

set BuildFolder=part02
:: set LinkerFlags=-opt:ref -incremental:no

:: Create build folder if it doesn't exist and change working directory
IF NOT EXIST %BuildFolder% mkdir %BuildFolder%
pushd %BuildFolder%

:: Compile test runner
cl %CompilerFlags% "haversine-generator.c"
popd
