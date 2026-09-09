@echo off
setlocal
set "C02_REPO=%CD%"
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b %errorlevel%
set "PATH=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin;%PATH%"
set "ASAN_OPTIONS=halt_on_error=1:exitcode=1"
cmake --preset asan -S Core_Study/exercises -B build/c02-final-asan-r2 -DCMAKE_MAKE_PROGRAM=D:/VisualStudio2026/Installed/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe -DCORE_STUDY_ENABLE_UNSAFE_DEMOS=ON
if errorlevel 1 exit /b %errorlevel%
cmake --build build/c02-final-asan-r2 --parallel 3
if errorlevel 1 exit /b %errorlevel%
ctest --test-dir build/c02-final-asan-r2 -L "^(reference|observation|capability)$" --output-on-failure --output-junit "%C02_REPO%\Core_Study\references\validation\integration\final-asan-safe-r2.xml"
if errorlevel 1 exit /b %errorlevel%
ctest --test-dir build/c02-final-asan-r2 -R "^L14_ub_asan_uaf_diagnostic$" --verbose --output-junit "%C02_REPO%\Core_Study\references\validation\integration\final-asan-unsafe-r2.xml"
exit /b %errorlevel%
