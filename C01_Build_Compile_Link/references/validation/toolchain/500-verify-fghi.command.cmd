@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1

set CMAKE=D:\cmake\install\bin\cmake.exe
set NINJA=D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe
set ROOT=F:\CPPTrain\LearnCPP
set EX=%ROOT%\Engineering_Study\exercises
set BUILD=%EX%\build
set GATE=d0edc3af-4c50-42ea-a356-e2862fe7a444

echo === F1 configure/build/test ===
"%CMAKE%" -S "%EX%\F1_cmake_targets" -B "%BUILD%\verify-f1" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 10
"%CMAKE%" --build "%BUILD%\verify-f1" --target F1_cmake_targets_reference --verbose
if errorlevel 1 exit /b 11
ctest --test-dir "%BUILD%\verify-f1" -R F1_cmake_targets_reference --output-on-failure
if errorlevel 1 exit /b 12

echo === F2 configure/build/test ===
"%CMAKE%" -S "%EX%\F2_dependencies" -B "%BUILD%\verify-f2" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 20
"%CMAKE%" --build "%BUILD%\verify-f2" --target F2_dependencies_reference --verbose
if errorlevel 1 exit /b 21
ctest --test-dir "%BUILD%\verify-f2" -R F2_dependencies_reference --output-on-failure
if errorlevel 1 exit /b 22

echo === G1 configure/build/test ===
"%CMAKE%" -S "%EX%\G1_diagnostics" -B "%BUILD%\verify-g1" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 30
"%CMAKE%" --build "%BUILD%\verify-g1" --target G1_diagnostics_reference --verbose
if errorlevel 1 exit /b 31
ctest --test-dir "%BUILD%\verify-g1" -R G1_diagnostics_reference --output-on-failure
if errorlevel 1 exit /b 32

echo === G2 configure/build/test ===
"%CMAKE%" -S "%EX%\G2_build_cost" -B "%BUILD%\verify-g2" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 40
"%CMAKE%" --build "%BUILD%\verify-g2" --target G2_build_cost_baseline G2_build_cost_pch G2_build_cost_lto --verbose
if errorlevel 1 exit /b 41
ctest --test-dir "%BUILD%\verify-g2" --output-on-failure
if errorlevel 1 exit /b 42

echo === H1 configure/build/test ===
"%CMAKE%" -S "%EX%\H1_modules" -B "%BUILD%\verify-h1" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 50
"%CMAKE%" --build "%BUILD%\verify-h1" --verbose
if errorlevel 1 exit /b 51
ctest --test-dir "%BUILD%\verify-h1" --output-on-failure
if errorlevel 1 exit /b 52

echo === I1 configure/build/test ===
"%CMAKE%" -S "%EX%\I1_import_std" -B "%BUILD%\verify-i1" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPERIMENTAL_CXX_IMPORT_STD=%GATE%
if errorlevel 1 exit /b 60
"%CMAKE%" --build "%BUILD%\verify-i1" --verbose
if errorlevel 1 exit /b 61
ctest --test-dir "%BUILD%\verify-i1" --output-on-failure
if errorlevel 1 exit /b 62

echo === done ===
exit /b 0

