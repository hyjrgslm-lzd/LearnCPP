@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b %errorlevel%
set "VSLANG=1033"
cmake -S Core_Study/references/validation/reviews/c02-l07-verifier-evidence-r1/fake-complete-bad -B build/c02-verifier-l07-fake-complete-bad-r1 -G Ninja -DCMAKE_MAKE_PROGRAM=D:/VisualStudio2026/Installed/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe -DCMAKE_CXX_COMPILER=D:/VisualStudio2026/Installed/VC/Tools/Llvm/x64/bin/clang++.exe -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b %errorlevel%
cmake --build build/c02-verifier-l07-fake-complete-bad-r1 --parallel 2
exit /b %errorlevel%
