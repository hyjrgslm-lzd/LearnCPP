@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1
set CMAKE=D:\cmake\install\bin\cmake.exe
set NINJA=D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe
set ROOT=F:\CPPTrain\LearnCPP
set G2=%ROOT%\Engineering_Study\exercises\G2_build_cost
set BUILD=%ROOT%\Engineering_Study\exercises\build\verify-g2-refactor
"%CMAKE%" -S "%G2%" -B "%BUILD%" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 10
"%CMAKE%" --build "%BUILD%" --target G2_build_cost_baseline G2_build_cost_pch G2_build_cost_lto --verbose
if errorlevel 1 exit /b 11
ctest --test-dir "%BUILD%" --output-on-failure
if errorlevel 1 exit /b 12
exit /b 0
