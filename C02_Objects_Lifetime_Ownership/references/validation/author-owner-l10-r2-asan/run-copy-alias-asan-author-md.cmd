@echo off
setlocal
set "LLVM_ASAN=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows"
set "PATH=%LLVM_ASAN%;%PATH%"
build\c02-owner-l10-r2-asan-md\alias-copy-uaf.exe
exit /b %errorlevel%
