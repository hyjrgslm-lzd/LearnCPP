@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1
set PYTHON=C:\Users\zhidan.li\AppData\Roaming\uv\python\cpython-3.13.11-windows-x86_64-none\python.exe
set ROOT=F:\CPPTrain\LearnCPP
if "%G2_OUTPUT%"=="" set G2_OUTPUT=%ROOT%\Engineering_Study\references\validation\toolchain\g2-preflight-author-r2
if "%G2_WORK_ROOT%"=="" set G2_WORK_ROOT=%ROOT%\Engineering_Study\exercises\build\g2-preflight-author-r2-work
"%PYTHON%" "%ROOT%\Engineering_Study\exercises\G2_build_cost\scripts\measure_build.py" --output "%G2_OUTPUT%" --work-root "%G2_WORK_ROOT%" --samples 1 --warmups 1 --seed 20260908 --parallel 1 --timeout 180
exit /b %errorlevel%
