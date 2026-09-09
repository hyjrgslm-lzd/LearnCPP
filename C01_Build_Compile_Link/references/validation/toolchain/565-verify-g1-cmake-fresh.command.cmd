@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1
set CMAKE=D:\cmake\install\bin\cmake.exe
set NINJA=D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe
set ROOT=F:\CPPTrain\LearnCPP
set G1=%ROOT%\Engineering_Study\exercises\G1_diagnostics
set BUILD=%ROOT%\Engineering_Study\exercises\build\verify-g1-r3-fresh
rmdir /s /q "%BUILD%" 2>nul
"%CMAKE%" -S "%G1%" -B "%BUILD%" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 10
"%CMAKE%" --build "%BUILD%" --target G1_diagnostics_reference --verbose
if errorlevel 1 exit /b 11
ctest --test-dir "%BUILD%" -R G1_diagnostics_reference --output-on-failure
if errorlevel 1 exit /b 12
exit /b 0
