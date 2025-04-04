:: Build script for haversine-gen.
:: IMPORTANT: "vcvarsall.bat" must be reachable via the PATH variable.

@echo off

:: Get the script's directory
set SCRIPT_FOLDER=%~dp0

:: Configure paths relative to the script's directory
set INCLUDES=-I%SCRIPT_FOLDER%..\common\src
set SOURCES=%SCRIPT_FOLDER%src\haversine-generator.c
set BUILD_FOLDER=%SCRIPT_FOLDER%bin
set OUT_EXE=haversine-generator
set LINKER_FLAGS=-incremental:no -opt:ref
set LIBS=

:: Optionally set debug mode here:
:: 0 for release, 1 for debug
set DEBUG=0

:: Set compiler flags based on debug/release build
IF [%DEBUG%] == [1] (
    set COMPILER_FLAGS=-nologo -Od -Gm- -MT -W4 -FC -wd4996 -wd4201 -wd4100 -wd4505 -DHAVERSINE_SLOW=1 -Zi -DEBUG:FULL
    set OUT_EXE=%OUT_EXE%_debug.exe
) ELSE (
    set COMPILER_FLAGS=-nologo -Od -Gm- -MT -W4 -FC -DHAVERSINE_SLOW=0
    set OUT_EXE=%OUT_EXE%_release.exe
)

:: Create build folder if it doesn't exist
IF NOT EXIST %BUILD_FOLDER% mkdir %BUILD_FOLDER%

:: It is still necessary to enter the build folder as CL produces additional build artifacts
:: in the current working directory.
pushd %BUILD_FOLDER%

:: Activate MSVC build environment if it hasn't been invoked yet
WHERE cl >nul 2>nul
IF NOT %ERRORLEVEL% == 0 (
    call vcvarsall.bat x64
)

:: Compile and link
cl %COMPILER_FLAGS% %INCLUDES% %SOURCES% -Fe"%BUILD_FOLDER%\%OUT_EXE%" /link %LINKER_FLAGS% %LIBS%

popd
