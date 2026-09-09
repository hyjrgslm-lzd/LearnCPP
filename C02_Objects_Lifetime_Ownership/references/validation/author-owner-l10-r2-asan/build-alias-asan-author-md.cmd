@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b %errorlevel%
set "LLVM_BIN=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin"
set "LLVM_ASAN=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows"
set "PATH=%LLVM_BIN%;%LLVM_ASAN%;%PATH%"
set "SRC=Core_Study\references\validation\reviews\c02-l10-alias-assign-asan-r1"
set "OUT=build\c02-owner-l10-r2-asan-md"
set "REF=Core_Study\exercises\L10_control_block\src\reference"
set "SUP=Core_Study\exercises\L10_control_block\checks\support"
if not exist "%OUT%" mkdir "%OUT%"
clang-cl /nologo /std:c++20 /EHsc /Od /Zi /fsanitize=address /MD /I "%REF%" /I "%SUP%" "%SRC%\alias-copy-uaf.cpp" /Fe:"%OUT%\alias-copy-uaf.exe"
if errorlevel 1 exit /b %errorlevel%
clang-cl /nologo /std:c++20 /EHsc /Od /Zi /fsanitize=address /MD /I "%REF%" /I "%SUP%" "%SRC%\alias-move-uaf.cpp" /Fe:"%OUT%\alias-move-uaf.exe"
exit /b %errorlevel%
