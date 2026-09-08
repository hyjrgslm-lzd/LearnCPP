@echo off
setlocal
set PYTHON=C:\Users\zhidan.li\AppData\Roaming\uv\python\cpython-3.13.11-windows-x86_64-none\python.exe
set ROOT=F:\CPPTrain\LearnCPP
"%PYTHON%" "%ROOT%\Engineering_Study\exercises\G2_build_cost\scripts\measure_build.py" --self-test
exit /b %errorlevel%
