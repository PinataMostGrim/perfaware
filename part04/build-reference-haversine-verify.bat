:: Build script for reference_haversine_verify.c

:: IMPORTANT: "vcvarsall.bat" must be reachable via the PATH variable.
:: Highly recommended to run batch file from the project's root folder.

:: Note: Uncomment this line to debug batch file commands
@echo off

:: Activate MSVC build environment if it hasn't been invoked yet
WHERE cl >nul 2>nul
IF NOT %ERRORLEVEL% == 0 (
    call vcvarsall.bat x64
)

setlocal

:: Save the script's folder in order to construct full paths for each source.
:: Some compilers seem to only output full paths on errors if this is done.
set SCRIPT_FOLDER=%~dp0

:: NOTE: Configure these variables
set SRC_FOLDER=src
set BUILD_FOLDER=bin
set OUT_EXE=reference_haversine_verify

set INCLUDES=-I%SCRIPT_FOLDER%..\common\src\ -I%SCRIPT_FOLDER%..\haversine\src\
set SOURCES="%SCRIPT_FOLDER%%SRC_FOLDER%\reference_haversine_verify.c"
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
IF NOT EXIST "%SCRIPT_FOLDER%%BUILD_FOLDER%" mkdir "%SCRIPT_FOLDER%%BUILD_FOLDER%"
pushd "%SCRIPT_FOLDER%%BUILD_FOLDER%"

:: Compile and link
cl %COMPILER_FLAGS% %INCLUDES% %SOURCES% -Fe%OUT_EXE% /link %LINKER_FLAGS% %LIBS%

popd
endlocal
