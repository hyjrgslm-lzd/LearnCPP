@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1
set CMAKE=D:\cmake\install\bin\cmake.exe
set NINJA=D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe
set ROOT=F:\CPPTrain\LearnCPP
set EX=%ROOT%\Engineering_Study\exercises
set GATE=d0edc3af-4c50-42ea-a356-e2862fe7a444
"%CMAKE%" -S "%EX%\I1_import_std" -B "%EX%\build\verify-i1-clean" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPERIMENTAL_CXX_IMPORT_STD=%GATE%
if errorlevel 1 exit /b 10
"%CMAKE%" --build "%EX%\build\verify-i1-clean" --verbose
if errorlevel 1 exit /b 11
ctest --test-dir "%EX%\build\verify-i1-clean" --output-on-failure
exit /b %errorlevel%
