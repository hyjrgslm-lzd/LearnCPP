@echo off
rem Recorded recipe for the verified Windows installation; run from LearnCPP root.
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b %errorlevel%
set "VSLANG=1033"
set "PATH=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin;D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows;%PATH%"
cmake -S Core_Study/references/validation/asan -B build/c02-asan-probe -G Ninja -DCMAKE_MAKE_PROGRAM=D:/VisualStudio2026/Installed/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe -DCMAKE_CXX_COMPILER=D:/VisualStudio2026/Installed/VC/Tools/Llvm/x64/bin/clang++.exe -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCORE_STUDY_ENABLE_ASAN=ON -DCORE_STUDY_ENABLE_UNSAFE_DEMOS=ON
if errorlevel 1 exit /b %errorlevel%
cmake --build build/c02-asan-probe --parallel 2
exit /b %errorlevel%
