@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1

set CMAKE=D:\cmake\install\bin\cmake.exe
set NINJA=D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe
set ROOT=F:\CPPTrain\LearnCPP
set F2=%ROOT%\Engineering_Study\exercises\F2_dependencies
set BUILD=%ROOT%\Engineering_Study\exercises\build\verify-f2-fetchcontent-fresh

rmdir /s /q "%BUILD%" 2>nul
"%CMAKE%" -S "%F2%" -B "%BUILD%" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release -DF2_DEPENDENCY_MODE=fetchcontent
if errorlevel 1 exit /b 10
"%CMAKE%" --build "%BUILD%" --target F2_dependencies_reference --verbose
if errorlevel 1 exit /b 11
ctest --test-dir "%BUILD%" -R F2_dependencies_reference --output-on-failure
if errorlevel 1 exit /b 12

echo === done ===
exit /b 0
