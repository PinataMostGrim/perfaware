:: IMPORTANT: "vcvarsall.bat x64" must be executed in the shell first. Must be run from project root folder.

@echo off

:: NOTE: Set %DEBUG% to 1 for debug build
IF [%DEBUG%] == [1] (
    :: Making debug build
    set CompilerFlags=-nologo -Od -Gm- -MT -W4 -FC -wd4996 -wd4201 -wd4100 -wd4505 -DSIM8086_SLOW=1 -Zi -DEBUG:FULL
) ELSE (
    :: Making release build
    set CompilerFlags=-nologo -Od -Gm- -MT -W4 -FC -DSIM8086_SLOW=0
)

set BuildFolder=sim8086\build
:: set LinkerFlags=-opt:ref -incremental:no
set PlatformLinkerFlags=-incremental:no -opt:ref User32.lib gdi32.lib winmm.lib

:: Create build folder if it doesn't exist and change working directory
IF NOT EXIST %BuildFolder% mkdir %BuildFolder%
pushd %BuildFolder%

:: Compile test runner
cl %CompilerFlags% "..\src\win32_sim8086.cpp" /link %PlatformLinkerFlags%
popd
