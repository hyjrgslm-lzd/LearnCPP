@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1

set CLANG=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin\clang++.exe
set RUNTIME=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows
set PATH=%RUNTIME%;%PATH%
set ROOT=F:\CPPTrain\LearnCPP
set G1=%ROOT%\Engineering_Study\exercises\G1_diagnostics
set OUT=%ROOT%\Engineering_Study\exercises\build\g1-r2-fuzz
set CORPUS=%G1%\fuzz_corpus
mkdir "%OUT%" 2>nul

echo === real parser fuzz harness finite run ===
"%CLANG%" -std=c++23 -fsanitize=fuzzer "%G1%\fuzz_parse.cpp" "%G1%\reference\parser.cpp" -I"%G1%\reference" -o "%OUT%\g1_real_parser_fuzz.exe"
if errorlevel 1 exit /b 10
"%OUT%\g1_real_parser_fuzz.exe" -runs=16 -seed=20260908 -max_total_time=5 "%CORPUS%"
if errorlevel 1 exit /b 11

echo === bad always-42 parser must be rejected ===
"%CLANG%" -std=c++23 -fsanitize=fuzzer "%G1%\fuzz_parse.cpp" "%G1%\fuzz_bad\always_42_parser.cpp" -I"%G1%\reference" -o "%OUT%\g1_bad_parser_fuzz.exe"
if errorlevel 1 exit /b 20
"%OUT%\g1_bad_parser_fuzz.exe" -runs=16 -seed=20260908 -max_total_time=5 "%CORPUS%"
if not errorlevel 1 exit /b 21

echo === done ===
exit /b 0
