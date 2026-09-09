@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1

set CLANG=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin\clang++.exe
set ROOT=F:\CPPTrain\LearnCPP
set G1=%ROOT%\Engineering_Study\exercises\G1_diagnostics
set OUT=%ROOT%\Engineering_Study\exercises\build\verify-static-analysis
mkdir "%OUT%" 2>nul

echo === clang static analyzer: reference parser ===
"%CLANG%" --analyze -std=c++23 "%G1%\reference\parser.cpp" -I"%G1%\reference" -o "%OUT%\parser.plist"
if errorlevel 1 exit /b 10

echo === clang static analyzer: nullable observation sample ===
"%CLANG%" --analyze -std=c++23 "%G1%\static_analysis\null_state.cpp" -o "%OUT%\null_state.plist"
if errorlevel 1 exit /b 20

echo === done ===
exit /b 0

