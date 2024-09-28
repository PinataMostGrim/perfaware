:: Build script for haversine-proc.
:: IMPORTANT: "vcvarsall.bat" must be reachable via the PATH variable.

@echo off

:: NOTE: Configure these variables
set INCLUDES=-I..\common\src
set SOURCES=src\haversine-processor.c
set LINKER_FLAGS=-incremental:no -opt:ref
set LIBS=

set BUILD_FOLDER=bin
set OUT_EXE=haversine-processor

:: NOTE: Set %DEBUG% to 1 for debug build
IF [%DEBUG%] == [1] (
    :: Making debug build
    set COMPILER_FLAGS=-nologo -Od -Gm- -MT -W4 -FC -wd4996 -wd4201 -wd4100 -wd4505 -DHAVERSINE_SLOW=1 -Zi -DEBUG:FULL
    set OUT_EXE=%OUT_EXE%_debug.exe
) ELSE (
    :: Making release build
    set COMPILER_FLAGS=-nologo -Od -Gm- -MT -W4 -FC -DHAVERSINE_SLOW=0
    :: Optimize code for speed
    :: set COMPILER_FLAGS=-nologo -Ot -Gm- -MT -W4 -FC -DHAVERSINE_SLOW=0
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

:: Compile and link
:: Execute this line instead to view application after the pre-processor has been applied
::cl -E %COMPILER_FLAGS% %INCLUDES% %SOURCES% -Fe%OUT_EXE% /link %LINKER_FLAGS% %LIBS% | clang-format -style="Microsoft" > temp.txt
cl %COMPILER_FLAGS% %INCLUDES% %SOURCES% -Fe%OUT_EXE% /link %LINKER_FLAGS% %LIBS%
popd