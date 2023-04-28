:: Runs tests on sim8086 using listings from the Performance-Aware Programming course
:: on Computer, Enhance (https://github.com/cmuratori/computer_enhance).

:: Requires the following components be placed in the same directory as this script:
:: - Compiled sim8068 executable
:: - Portable version of NASM
:: - All Performance-Aware Programming listings included in the test suite

echo off

set /a PASSED=0
set /a FAILED=0

echo.
echo Running sim8068 test suite:
echo ---------------------------

: Run tests
call :TestListing listing_0037_single_register_mov
call :TestListing listing_0038_many_register_mov
call :TestListing listing_0039_more_movs
call :TestListing listing_0040_challenge_movs
call :TestListing listing_0041_add_sub_cmp_jnz
call :TestListing listing_0043_immediate_movs
call :TestListing listing_0044_register_movs
call :TestListing listing_0046_add_sub_cmp
call :TestListing listing_0048_ip_register
call :TestListing listing_0049_conditional_jumps
call :TestListing listing_0051_memory_mov
call :TestListing listing_0052_memory_add_loop
call :TestListing listing_0053_add_loop_challenge


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
sim8086 %LISTING% > output.asm
nasm output.asm -o output
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
