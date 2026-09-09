@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1

set CMAKE=D:\cmake\install\bin\cmake.exe
set NINJA=D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe
set PROBE=F:\CPPTrain\LearnCPP\Engineering_Study\exercises\build\toolchain-probe
set GATE=d0edc3af-4c50-42ea-a356-e2862fe7a444

echo === named configure ===
"%CMAKE%" -S "%PROBE%\msvc-named-modules" -B "%PROBE%\out\normal-named" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 10
echo === named build ===
"%CMAKE%" --build "%PROBE%\out\normal-named" --verbose
if errorlevel 1 exit /b 11
echo === named run ===
"%PROBE%\out\normal-named\named_modules_probe.exe"
if errorlevel 1 exit /b 12

echo === import std configure ===
"%CMAKE%" -S "%PROBE%\msvc-import-std" -B "%PROBE%\out\normal-import-std" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPERIMENTAL_CXX_IMPORT_STD=%GATE%
if errorlevel 1 exit /b 20
echo === import std build ===
"%CMAKE%" --build "%PROBE%\out\normal-import-std" --verbose
if errorlevel 1 exit /b 21
echo === import std run ===
"%PROBE%\out\normal-import-std\import_std_probe.exe"
if errorlevel 1 exit /b 22

echo === module package configure ===
"%CMAKE%" -S "%PROBE%\msvc-module-install\library" -B "%PROBE%\out\normal-module-install" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%PROBE%\out\normal-module-package"
if errorlevel 1 exit /b 30
echo === module package build ===
"%CMAKE%" --build "%PROBE%\out\normal-module-install" --verbose
if errorlevel 1 exit /b 31
echo === module package install ===
"%CMAKE%" --install "%PROBE%\out\normal-module-install"
if errorlevel 1 exit /b 32

echo === module consumer configure ===
"%CMAKE%" -S "%PROBE%\msvc-module-install\consumer" -B "%PROBE%\out\normal-module-consumer" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%PROBE%\out\normal-module-package"
if errorlevel 1 exit /b 40
echo === module consumer build ===
"%CMAKE%" --build "%PROBE%\out\normal-module-consumer" --verbose
if errorlevel 1 exit /b 41
echo === module consumer run ===
"%PROBE%\out\normal-module-consumer\module_consumer_probe.exe"
if errorlevel 1 exit /b 42

echo === done ===
exit /b 0
