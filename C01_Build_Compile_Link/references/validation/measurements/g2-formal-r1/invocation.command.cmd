@echo off
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1
set VSLANG=1033
"C:\Users\zhidan.li\AppData\Roaming\uv\python\cpython-3.13.11-windows-x86_64-none\python.exe" "F:\CPPTrain\LearnCPP\Engineering_Study\exercises\G2_build_cost\scripts\measure_build.py" --output "F:\CPPTrain\LearnCPP\Engineering_Study\references\validation\measurements\g2-formal-r1" --work-root "F:\CPPTrain\LearnCPP\Engineering_Study\exercises\build\g2-formal-r1-work" --samples 5 --warmups 1 --seed 20260908 --parallel 1 --timeout 180 --cmake "D:\cmake\install\bin\cmake.exe" --ninja "D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
exit /b %errorlevel%
