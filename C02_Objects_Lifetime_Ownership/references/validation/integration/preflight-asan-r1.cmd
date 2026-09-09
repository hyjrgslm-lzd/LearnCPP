@echo off
rem Recorded Windows installation. Run from LearnCPP root; environment is child-local.
setlocal
set "C02_REPO=%CD%"
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b %errorlevel%
set "VSLANG=1033"
set "PATH=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin;D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows;%PATH%"
set "ASAN_OPTIONS=halt_on_error=1:exitcode=1"
cmake --preset asan -S Core_Study/exercises -DCMAKE_MAKE_PROGRAM=D:/VisualStudio2026/Installed/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe
if errorlevel 1 exit /b %errorlevel%
cmake --build Core_Study/exercises/build/asan --parallel 3
if errorlevel 1 exit /b %errorlevel%
pushd Core_Study\exercises
ctest --preset asan --output-junit "%C02_REPO%\Core_Study\references\validation\integration\preflight-asan-tests-r1.xml"
set "C02_RESULT=%errorlevel%"
popd
exit /b %C02_RESULT%
