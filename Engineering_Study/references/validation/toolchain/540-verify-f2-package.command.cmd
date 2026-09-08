@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1

set CMAKE=D:\cmake\install\bin\cmake.exe
set NINJA=D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe
set ROOT=F:\CPPTrain\LearnCPP
set F2=%ROOT%\Engineering_Study\exercises\F2_dependencies
set BUILD=%ROOT%\Engineering_Study\exercises\build
set PREFIX=%BUILD%\verify-f2-provider-install

rmdir /s /q "%BUILD%\verify-f2-provider" 2>nul
rmdir /s /q "%PREFIX%" 2>nul
rmdir /s /q "%BUILD%\verify-f2-find" 2>nul
rmdir /s /q "%BUILD%\verify-f2-mismatch" 2>nul
mkdir "%BUILD%\verify-f2-mismatch" 2>nul

"%CMAKE%" -S "%F2%\provider_fixture" -B "%BUILD%\verify-f2-provider" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release "-DCMAKE_INSTALL_PREFIX=%PREFIX%"
if errorlevel 1 exit /b 10
"%CMAKE%" --build "%BUILD%\verify-f2-provider" --target install --verbose
if errorlevel 1 exit /b 11

"%CMAKE%" -S "%F2%" -B "%BUILD%\verify-f2-find" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release -DF2_DEPENDENCY_MODE=find_package "-DCMAKE_PREFIX_PATH=%PREFIX%"
if errorlevel 1 exit /b 20
"%CMAKE%" --build "%BUILD%\verify-f2-find" --target F2_dependencies_reference --verbose
if errorlevel 1 exit /b 21
ctest --test-dir "%BUILD%\verify-f2-find" -R F2_dependencies_reference --output-on-failure
if errorlevel 1 exit /b 22

> "%BUILD%\verify-f2-mismatch\CMakeLists.txt" echo cmake_minimum_required(VERSION 3.28)
>> "%BUILD%\verify-f2-mismatch\CMakeLists.txt" echo project(F2Mismatch LANGUAGES CXX)
>> "%BUILD%\verify-f2-mismatch\CMakeLists.txt" echo find_package(F2Provider 2.0 CONFIG REQUIRED)
"%CMAKE%" -S "%BUILD%\verify-f2-mismatch" -B "%BUILD%\verify-f2-mismatch-build" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" "-DCMAKE_PREFIX_PATH=%PREFIX%"
if not errorlevel 1 exit /b 30

echo === done ===
exit /b 0
