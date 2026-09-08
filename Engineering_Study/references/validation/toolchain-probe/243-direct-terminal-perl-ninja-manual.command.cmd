@echo off
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
cd /d "F:\CPPTrain\LearnCPP\Engineering_Study\exercises\build\toolchain-probe\ninja-manual"
"D:\Perl\c\bin\ninja.exe" -v
