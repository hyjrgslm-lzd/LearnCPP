@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b %errorlevel%
set "VSLANG=1033"
cmake -S Core_Study/references/validation/reviews/c02-l07-verifier-evidence-r3/fake-support-shadow -B build/c02-verifier-l07-r3-fake-support-shadow -G "Visual Studio 18 2026" -A x64
if errorlevel 1 exit /b %errorlevel%
cmake --build build/c02-verifier-l07-r3-fake-support-shadow --config Debug
exit /b %errorlevel%
