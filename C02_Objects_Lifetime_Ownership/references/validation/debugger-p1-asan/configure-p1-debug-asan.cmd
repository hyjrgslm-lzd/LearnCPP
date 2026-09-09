@echo off
rem Recorded recipe for P1 Debug AddressSanitizer diagnosis; run from LearnCPP root.
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b %errorlevel%
set "VSLANG=1033"
set "PATH=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin;D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows;%PATH%"
cmake -S Core_Study/exercises/P1_object_buffer -B build/c02-asan-debugger/p1-asan-debug -G Ninja -DCMAKE_MAKE_PROGRAM=D:/VisualStudio2026/Installed/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe -DCMAKE_CXX_COMPILER=D:/VisualStudio2026/Installed/VC/Tools/Llvm/x64/bin/clang++.exe -DCMAKE_BUILD_TYPE=Debug -DCORE_STUDY_ENABLE_ASAN=ON
if errorlevel 1 exit /b %errorlevel%
cmake --build build/c02-asan-debugger/p1-asan-debug --target P1_object_buffer_reference --parallel 2
exit /b %errorlevel%
