@echo off
setlocal EnableDelayedExpansion
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1
set "CMAKE=D:\cmake\install\bin\cmake.exe"
set "NINJA=D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
set "ROOT=F:\CPPTrain\LearnCPP"
set "F2=%ROOT%\Engineering_Study\exercises\F2_dependencies"
set "BUILD=%ROOT%\Engineering_Study\exercises\build\verify-f2-r2-fetchcontent-local"
set "BAD=%ROOT%\Engineering_Study\exercises\build\verify-f2-r2-fetchcontent-missing-source"
set "LOG=%ROOT%\Engineering_Study\references\validation\toolchain\f2-r2-fetchcontent-local"
rmdir /s /q "%BUILD%" 2>nul
rmdir /s /q "%BAD%" 2>nul
rmdir /s /q "%LOG%" 2>nul
mkdir "%LOG%" >nul
"%CMAKE%" -S "%F2%" -B "%BUILD%" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release -DF2_DEPENDENCY_MODE=fetchcontent > "%LOG%\valid-configure.stdout.txt" 2> "%LOG%\valid-configure.stderr.txt"
if errorlevel 1 exit /b 10
"%CMAKE%" --build "%BUILD%" --target F2_dependencies_reference --verbose > "%LOG%\valid-build.stdout.txt" 2> "%LOG%\valid-build.stderr.txt"
if errorlevel 1 exit /b 11
ctest --test-dir "%BUILD%" -R F2_dependencies_reference --output-on-failure > "%LOG%\valid-ctest.stdout.txt" 2> "%LOG%\valid-ctest.stderr.txt"
if errorlevel 1 exit /b 12
"%CMAKE%" -S "%F2%" -B "%BAD%" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release -DF2_DEPENDENCY_MODE=fetchcontent "-DF2_PROVIDER_FIXTURE_SOURCE_DIR=%ROOT%\Engineering_Study\exercises\build\missing-f2-provider-fixture" > "%LOG%\missing-configure.stdout.txt" 2> "%LOG%\missing-configure.stderr.txt"
if not errorlevel 1 exit /b 13
findstr /c:"F2_PROVIDER_FIXTURE_SOURCE_DIR" "%LOG%\missing-configure.stdout.txt" >nul
if errorlevel 1 findstr /c:"F2_PROVIDER_FIXTURE_SOURCE_DIR" "%LOG%\missing-configure.stderr.txt" >nul
if errorlevel 1 exit /b 14
exit /b 0
