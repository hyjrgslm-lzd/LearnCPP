@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1
set PYTHON=C:\Users\zhidan.li\AppData\Roaming\uv\python\cpython-3.13.11-windows-x86_64-none\python.exe
set ROOT=F:\CPPTrain\LearnCPP
set OUT=%ROOT%\Engineering_Study\references\validation\toolchain\g2-r4-subset-smoke
set WORK=%ROOT%\Engineering_Study\exercises\build\g2-r4-subset-smoke-work
"%PYTHON%" "%ROOT%\Engineering_Study\exercises\G2_build_cost\scripts\measure_build.py" --output "%OUT%" --work-root "%WORK%" --samples 1 --warmups 1 --seed 20260908 --parallel 1 --timeout 180 --variant baseline --scenario noop
exit /b %errorlevel%
