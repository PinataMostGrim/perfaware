:: IMPORTANT: "vcvarsall.bat" must be reachable via the PATH variable.
:: Highly recommended to run batch file from the project's root folder.

@echo off

:: NOTE: Configure these variables
set INCLUDES=-I..\..\common\src -I..\src\imgui -I..\src\imgui\backends
set SOURCES=
set IMGUI_SOURCES=..\src\imgui\imgui*.cpp ..\src\imgui\backends\imgui_impl_opengl3.cpp ..\src\imgui\backends\imgui_impl_win32.cpp
set LINKER_FLAGS=-incremental:no -opt:ref
set LIBS=User32.lib gdi32.lib winmm.lib opengl32.lib sim8086_application.lib
set IMGUI_OBJS=imgui*.obj

set GUI_LOCK_FILE=sim8086_lock.tmp

set BUILD_FOLDER=sim8086\bin
set OUT_EXE=sim8086_win32

:: 0 for disabled, 1 for enabled
set DIAGNOSTICS=1

:: NOTE: Set %DEBUG% to 1 for debug build
IF [%DEBUG%] == [1] (
    :: Making debug build
    set COMPILER_FLAGS=-nologo -Od -Gm- -MT -W4 -FC -wd4996 -wd4201 -wd4100 -wd4505 -wd4189 -wd4127 -DSIM8086_SLOW=1 -DSIM8086_DIAGNOSTICS=%DIAGNOSTICS% -Zi -DEBUG:FULL
    set OUT_EXE=%OUT_EXE%_debug.exe
) ELSE (
    :: Making release build
    set COMPILER_FLAGS=-nologo -Od -Gm- -MT -W4 -FC -DSIM8086_SLOW=0 -DSIM8086_DIAGNOSTICS=%DIAGNOSTICS%
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

:: TODO: Only compile IMGUI files if the objects don't already exist

del *.pdb > NUL 2> NUL
echo WAITING FOR PDB > %GUI_LOCK_FILE%
:: Compile application layer
cl %COMPILER_FLAGS% %INCLUDES% ..\src\sim8086_application.cpp %IMGUI_SOURCES% -Fmsim8086_application.map -LD /link %LINKER_FLAGS% -PDB:sim8086_application_%random%.pdb -EXPORT:SetImGuiContext -EXPORT:UpdateAndRender
:: Execute this line instead to view application layer after the pre-processor has been applied
:: cl -E %COMPILER_FLAGS% %INCLUDES% ..\src\sim8086_application.cpp %IMGUI_SOURCES% -Fmsim8086_application.map -LD /link %LINKER_FLAGS% -PDB:sim8086_application_%random%.pdb -EXPORT:SetImGuiContext -EXPORT:UpdateAndRender | clang-format -style="Microsoft" > temp.txt
del %GUI_LOCK_FILE%

:: Compile platform layer
cl %COMPILER_FLAGS% %INCLUDES% ..\src\sim8086_win32.cpp -Fe%OUT_EXE% /link %LINKER_FLAGS% %LIBS% %IMGUI_OBJS%
:: Execute this line instead to view platform layer after the pre-processor has been applied
:: cl -E %COMPILER_FLAGS% %INCLUDES% ..\src\sim8086_win32.cpp -Fe%OUT_EXE% /link %LINKER_FLAGS% %LIBS% %IMGUI_OBJS% | clang-format -style="Microsoft" > temp.txt
popd
