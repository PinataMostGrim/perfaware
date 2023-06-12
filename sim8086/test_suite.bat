:: Runs tests on sim8086 using listings from the Performance-Aware Programming course
:: on Computer, Enhance (https://github.com/cmuratori/computer_enhance).

:: Requires the following components be placed in the correct folders relative to this script:
:: - 'build\sim8086.exe'
:: - 'ext\nasm.exe'
:: - All Performance-Aware Programming listings included in the test suite placed in 'listings\'

echo off

set /a PASSED=0
set /a FAILED=0

echo.
echo Running sim8068 test suite:
echo ---------------------------

: Run tests
call :TestListing listings\listing_0037_single_register_mov
call :TestListing listings\listing_0038_many_register_mov
call :TestListing listings\listing_0039_more_movs
call :TestListing listings\listing_0040_challenge_movs
call :TestListing listings\listing_0041_add_sub_cmp_jnz
call :TestListing listings\listing_0043_immediate_movs
call :TestListing listings\listing_0044_register_movs
call :TestListing listings\listing_0046_add_sub_cmp
call :TestListing listings\listing_0048_ip_register
call :TestListing listings\listing_0049_conditional_jumps
call :TestListing listings\listing_0051_memory_mov
call :TestListing listings\listing_0052_memory_add_loop
call :TestListing listings\listing_0053_add_loop_challenge
call :TestListing listings\listing_0054_draw_rectangle
call :TestListing listings\listing_0056_estimating_cycles


:: Output results
echo.
echo PASSED: %PASSED%
echo FAILED: %FAILED%

:: Clean up
del output.asm
del output

:: Exit
goto :eof


:: Test function
:TestListing
SET LISTING=%~1
echo Testing '%LISTING%':
echo
:: TODO: Need to update this
build\sim8086.exe %LISTING% > output.asm
ext\nasm.exe output.asm -o output
fc /B %LISTING% output || goto :error
set /a PASSED=PASSED+1
echo PASSED
echo ------------------------
goto :eof


:error
set /a FAILED=FAILED+1
echo.
echo                   FAILED
echo ------------------------
exit /b %errorlevel%
