:: IMPORTANT: "vcvarsall.bat" must be reachable via the PATH variable.
:: Highly recommended to run batch file from the project's root folder.

@echo off

:: NOTE: Configure these variables
set INCLUDES=
set SOURCES=..\src\haversine-processor.c
set LINKER_FLAGS=-incremental:no -opt:ref
set LIBS=

set BUILD_FOLDER=haversine\build
set OUT_EXE=haversine-processor

:: NOTE: Set %DEBUG% to 1 for debug build
IF [%DEBUG%] == [1] (
    :: Making debug build
    set COMPILER_FLAGS=-nologo -Od -Gm- -MT -W4 -FC -wd4996 -wd4201 -wd4100 -wd4505 -DHAVERSINE_SLOW=1 -Zi -DEBUG:FULL
    set OUT_EXE=%OUT_EXE%_debug.exe
) ELSE (
    :: Making release build
    set COMPILER_FLAGS=-nologo -Od -Gm- -MT -W4 -FC -DHAVERSINE_SLOW=0
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
