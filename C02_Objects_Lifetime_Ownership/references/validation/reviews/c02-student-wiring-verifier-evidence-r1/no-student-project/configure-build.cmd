@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b %errorlevel%
set "VSLANG=1033"
set "BUILD_DIR=build/c02-verifier-wiring-no-student-r1"
mkdir "%BUILD_DIR%\.cmake\api\v1\query\client-c02" 2>nul
type nul > "%BUILD_DIR%\.cmake\api\v1\query\client-c02\codemodel-v2"
cmake -S Core_Study/references/validation/reviews/c02-student-wiring-verifier-evidence-r1/no-student-project -B "%BUILD_DIR%" -G "Visual Studio 18 2026" -A x64
if errorlevel 1 exit /b %errorlevel%
cmake --build "%BUILD_DIR%" --config Release --clean-first --target plain_target
exit /b %errorlevel%
