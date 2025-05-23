@echo off

:: Build script for test_2x_faster_file_reads.c

:: IMPORTANT: "vcvarsall.bat" must be reachable via the PATH variable.
:: Highly recommended to run batch file from the project's root folder.

:: Note: Uncomment to debug commands
:: @echo on

:: Save the script's folder in order to construct full paths for each source.
:: Some compilers seem to only output full paths on errors if this is done.
set SCRIPT_FOLDER=%~dp0

:: NOTE: Configure these variables
set SRC_FOLDER=src
set BUILD_FOLDER=bin
set OUT_EXE=test_2x_faster_file_reads

:: TODO (Aaron): Update the includes to point to common/src when next on a windows machine
set INCLUDES=
set SOURCES="%SCRIPT_FOLDER%%SRC_FOLDER%\test_2x_faster_file_reads.c"
set LINKER_FLAGS=-incremental:no -opt:ref
set LIBS=

:: Optionally set debug mode here:
set DEBUG=1


:: NOTE: Set %DEBUG% to 1 for debug build
IF [%DEBUG%] == [1] (
    :: Making debug build
    set COMPILER_FLAGS=-nologo -Od -Gm- -MT -W4 -FC -wd4996 -wd4201 -wd4100 -wd4505 -Zi -DEBUG:FULL
    set OUT_EXE=%OUT_EXE%_debug.exe
) ELSE (
    :: Making release build
    set COMPILER_FLAGS=-nologo -Od -Gm- -MT -W4 -FC
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
cl %COMPILER_FLAGS% %INCLUDES% %SOURCES% -Fe%OUT_EXE% /link %LINKER_FLAGS% %LIBS%
popd
