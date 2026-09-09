@echo off
setlocal EnableDelayedExpansion
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1
set "CMAKE=D:\cmake\install\bin\cmake.exe"
set "NINJA=D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
set "ROOT=F:\CPPTrain\LearnCPP"
set "G1=%ROOT%\Engineering_Study\exercises\G1_diagnostics"
set "BASE=%ROOT%\Engineering_Study\references\validation\toolchain\g1-safe-starter-r2"
rmdir /s /q "%BASE%" 2>nul
mkdir "%BASE%" >nul
for %%C in (Debug Release) do call :check_one %%C
exit /b 0

:check_one
set "CONFIG=%1"
set "BUILD=%ROOT%\Engineering_Study\exercises\build\g1-safe-starter-r2-%CONFIG%"
rmdir /s /q "%BUILD%" 2>nul
"%CMAKE%" -S "%G1%" -B "%BUILD%" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" "-DCMAKE_BUILD_TYPE=%CONFIG%" -DENGINEERING_STUDY_TEST_STUDENTS=ON > "%BASE%\%CONFIG%-configure.stdout.txt" 2> "%BASE%\%CONFIG%-configure.stderr.txt"
if errorlevel 1 exit /b 10
"%CMAKE%" --build "%BUILD%" --target G1_diagnostics_reference G1_diagnostics_student --verbose > "%BASE%\%CONFIG%-build.stdout.txt" 2> "%BASE%\%CONFIG%-build.stderr.txt"
if errorlevel 1 exit /b 11
ctest --test-dir "%BUILD%" -R G1_diagnostics_reference --output-on-failure > "%BASE%\%CONFIG%-reference-ctest.stdout.txt" 2> "%BASE%\%CONFIG%-reference-ctest.stderr.txt"
if errorlevel 1 exit /b 12
"%BUILD%\G1_diagnostics_student.exe" > "%BASE%\%CONFIG%-student.stdout.txt" 2> "%BASE%\%CONFIG%-student.stderr.txt"
set "STUDENT_EXIT=!ERRORLEVEL!"
> "%BASE%\%CONFIG%-student.exit.txt" echo(!STUDENT_EXIT!
if not "!STUDENT_EXIT!"=="1" exit /b 13
findstr /c:"student parser must parse valid input" "%BASE%\%CONFIG%-student.stderr.txt" >nul
if errorlevel 1 exit /b 14
exit /b 0
