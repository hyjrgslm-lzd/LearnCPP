@echo off
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
"D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S "F:\CPPTrain\LearnCPP\Engineering_Study\exercises\build\toolchain-probe\cmake-plain" -B "F:\CPPTrain\LearnCPP\Engineering_Study\exercises\build\toolchain-probe\out\vs-cmake-plain" -G Ninja -DCMAKE_MAKE_PROGRAM="D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" -DCMAKE_BUILD_TYPE=Release
