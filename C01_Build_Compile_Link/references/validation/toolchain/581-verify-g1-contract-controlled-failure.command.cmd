@echo off
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
python "F:\CPPTrain\LearnCPP\Engineering_Study\references\validation\toolchain\581-verify-g1-contract-controlled-failure.py"
