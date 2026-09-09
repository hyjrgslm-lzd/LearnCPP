@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1
set PYTHON=C:\Users\zhidan.li\AppData\Roaming\uv\python\cpython-3.13.11-windows-x86_64-none\python.exe
set ROOT=F:\CPPTrain\LearnCPP
"%PYTHON%" "%ROOT%\Engineering_Study\exercises\G1_diagnostics\scripts\verify_fuzz.py" --output "%ROOT%\Engineering_Study\references\validation\toolchain\g1-r3-fuzz-real-parser"
exit /b %errorlevel%
