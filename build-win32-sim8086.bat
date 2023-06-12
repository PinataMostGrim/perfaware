:: IMPORTANT: "vcvarsall.bat" must be reachable via the PATH variable.
:: Highly recommended to run batch file from the project's root folder.

@echo off

:: NOTE: Configure these variables
set INCLUDES=-I..\src\imgui -I..\src\imgui\backends
set SOURCES=..\src\win32_sim8086.cpp ..\src\imgui\imgui*.cpp ..\src\imgui\backends\imgui_impl_opengl3.cpp ..\src\imgui\backends\imgui_impl_win32.cpp
set LINKER_FLAGS=-incremental:no -opt:ref
set LIBS=User32.lib gdi32.lib winmm.lib opengl32.lib

set BUILD_FOLDER=sim8086\build
set OUT_EXE=win32_sim8086

:: NOTE: Set %DEBUG% to 1 for debug build
IF [%DEBUG%] == [1] (
    :: Making debug build
    set COMPILER_FLAGS=-nologo -Od -Gm- -MT -W4 -FC -wd4996 -wd4201 -wd4100 -wd4505 -DSIM8086_SLOW=1 -Zi -DEBUG:FULL
    set OUT_EXE=%OUT_EXE%_debug.exe
) ELSE (
    :: Making release build
    set COMPILER_FLAGS=-nologo -Od -Gm- -MT -W4 -FC -DSIM8086_SLOW=0
    set OUT_EXE=%OUT_EXE%_release.exe
)

:: Create build folder if it doesn't exist and change working directory
IF NOT EXIST %BUILD_FOLDER% mkdir %BUILD_FOLDER%
pushd %BUILD_FOLDER%

:: Activate MSVC build environment if it hasn't been invoked yet
WHERE cl >nul 2>nul
IF NOT %ERRORLEVEL% == 0 (
    call vcvarsall.bat x64
)

:: Compile test runner
cl %COMPILER_FLAGS% %INCLUDES% %SOURCES% -Fe%OUT_EXE% /link %LINKER_FLAGS% %LIBS%
popd
