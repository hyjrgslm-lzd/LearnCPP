@echo off
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
where cl
where link
cd /d "F:\CPPTrain\LearnCPP\Engineering_Study\exercises\build\toolchain-probe\ninja-manual"
"D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" -v
