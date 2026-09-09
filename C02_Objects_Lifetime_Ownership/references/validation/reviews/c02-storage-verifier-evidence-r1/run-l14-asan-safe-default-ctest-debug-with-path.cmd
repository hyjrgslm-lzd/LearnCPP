@echo off
set "PATH=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin;D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows;%PATH%"
set "ASAN_OPTIONS=halt_on_error=1:exitcode=1"
ctest --test-dir build/c02-storage-verifier-r1/L14-asan-safe-default -C Debug --output-on-failure
