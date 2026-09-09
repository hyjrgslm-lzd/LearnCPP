@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b %errorlevel%
set "LLVM_BIN=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin"
set "LLVM_ASAN=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows"
set "PATH=%LLVM_BIN%;%LLVM_ASAN%;%PATH%"
where clang-cl
clang-cl --version
if errorlevel 1 exit /b %errorlevel%
set "OUT=Core_Study\references\validation\reviews\c02-0810-r2-verifier-evidence-r1"
set "REF=Core_Study\exercises\L10_control_block\src\reference"
set "SUP=Core_Study\exercises\L10_control_block\checks\support"
clang-cl /nologo /std:c++20 /EHsc /Od /Zi /fsanitize=address /MD /I "%REF%" /I "%SUP%" "%OUT%\alias-copy-uaf-r2.cpp" /Fe:"%OUT%\alias-copy-uaf-r2.exe"
if errorlevel 1 exit /b %errorlevel%
clang-cl /nologo /std:c++20 /EHsc /Od /Zi /fsanitize=address /MD /I "%REF%" /I "%SUP%" "%OUT%\alias-move-uaf-r2.cpp" /Fe:"%OUT%\alias-move-uaf-r2.exe"
exit /b %errorlevel%
