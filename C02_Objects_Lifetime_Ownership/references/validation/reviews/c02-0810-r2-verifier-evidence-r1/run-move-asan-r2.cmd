@echo off
setlocal
set "LLVM_BIN=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin"
set "LLVM_ASAN=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows"
set "PATH=%LLVM_BIN%;%LLVM_ASAN%;%PATH%"
set "ASAN_OPTIONS=halt_on_error=1:exitcode=1"
"%~dp0alias-move-uaf-r2.exe"
exit /b %errorlevel%
