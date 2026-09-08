@echo off
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
"D:\cmake\install\bin\cmake.exe" -S "F:\CPPTrain\LearnCPP\Engineering_Study\exercises\build\toolchain-probe\cmake-plain" -B "F:\CPPTrain\LearnCPP\Engineering_Study\exercises\build\toolchain-probe\out\diag-z7" -G Ninja -DCMAKE_MAKE_PROGRAM="D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_DEFAULT_CMP0141=NEW -DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded
