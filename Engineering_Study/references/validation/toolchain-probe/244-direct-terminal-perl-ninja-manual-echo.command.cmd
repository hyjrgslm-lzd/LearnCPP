@echo off
echo before-vsdev
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
echo after-vsdev
where cl
where ninja
cd /d "F:\CPPTrain\LearnCPP\Engineering_Study\exercises\build\toolchain-probe\ninja-manual"
echo before-ninja
"D:\Perl\c\bin\ninja.exe" -v
echo after-ninja %ERRORLEVEL%
