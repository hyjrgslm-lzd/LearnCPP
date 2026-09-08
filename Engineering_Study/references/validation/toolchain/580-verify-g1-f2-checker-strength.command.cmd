@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1
set PYTHON=C:\Users\zhidan.li\AppData\Roaming\uv\python\cpython-3.13.11-windows-x86_64-none\python.exe
set ROOT=F:\CPPTrain\LearnCPP
"%PYTHON%" "%ROOT%\Engineering_Study\references\validation\toolchain\580-verify-g1-f2-checker-strength.py" --output "%ROOT%\Engineering_Study\references\validation\toolchain\g1-f2-checker-strength-r1" --work-root "%ROOT%\Engineering_Study\exercises\build\g1-f2-checker-strength-r1-work"
exit /b %errorlevel%
