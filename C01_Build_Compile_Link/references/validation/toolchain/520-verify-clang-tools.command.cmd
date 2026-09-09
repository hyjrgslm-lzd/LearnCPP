@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1

set CLANG=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin\clang++.exe
set RUNTIME=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows
set PATH=%RUNTIME%;%PATH%
set ROOT=F:\CPPTrain\LearnCPP
set G1=%ROOT%\Engineering_Study\exercises\G1_diagnostics
set G2=%ROOT%\Engineering_Study\exercises\G2_build_cost
set OUT=%ROOT%\Engineering_Study\exercises\build\verify-clang-tools
mkdir "%OUT%" 2>nul
set FUZZ_CORPUS=%OUT%\g1-fuzz-corpus
rmdir /s /q "%FUZZ_CORPUS%" 2>nul
mkdir "%FUZZ_CORPUS%" 2>nul
copy /y "%G1%\fuzz_corpus\*" "%FUZZ_CORPUS%\" >nul

echo === asan safe build/run ===
"%CLANG%" -std=c++23 -fsanitize=address -g "%G1%\reference\parser.cpp" "%G1%\checks\reference_check.cpp" -I"%G1%\reference" -I"%ROOT%\Engineering_Study\exercises\include" -o "%OUT%\g1_asan_safe.exe"
if errorlevel 1 exit /b 10
"%OUT%\g1_asan_safe.exe"
if errorlevel 1 exit /b 11

echo === asan unsafe build/run expected failure ===
"%CLANG%" -std=c++23 -fsanitize=address -g "%G1%\unsafe\asan_fault.cpp" -o "%OUT%\g1_asan_fault.exe"
if errorlevel 1 exit /b 20
"%OUT%\g1_asan_fault.exe"
if not errorlevel 1 exit /b 21

echo === libfuzzer finite runs ===
"%CLANG%" -std=c++23 -fsanitize=fuzzer "%G1%\fuzz_parse.cpp" "%G1%\reference\parser.cpp" -I"%G1%\reference" -o "%OUT%\g1_fuzz_parse.exe"
if errorlevel 1 exit /b 30
"%OUT%\g1_fuzz_parse.exe" -runs=16 -seed=20260908 -max_total_time=5 "%FUZZ_CORPUS%"
if errorlevel 1 exit /b 31

echo === ftime trace ===
"%CLANG%" -std=c++23 -ftime-trace "%G2%\reference\alpha.cpp" "%G2%\reference\beta.cpp" "%G2%\reference\gamma.cpp" "%G2%\reference\common.cpp" "%G2%\reference\main.cpp" -I"%G2%\reference\include" -I"%ROOT%\Engineering_Study\exercises\include" -o "%OUT%\g2_trace.exe"
if errorlevel 1 exit /b 40
dir /b "%OUT%\*.json"
if errorlevel 1 exit /b 41

echo === done ===
exit /b 0



