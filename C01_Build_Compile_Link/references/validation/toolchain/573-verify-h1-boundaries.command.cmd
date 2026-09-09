@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1
set CMAKE=D:\cmake\install\bin\cmake.exe
set NINJA=D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe
set ROOT=F:\CPPTrain\LearnCPP
set H1=%ROOT%\Engineering_Study\exercises\H1_modules
set BUILD=%ROOT%\Engineering_Study\exercises\build\verify-h1-r4-boundaries-573
set NEGOUT=%ROOT%\Engineering_Study\references\validation\toolchain\573-h1-hidden-type-negative.stdout.txt
set NEGERR=%ROOT%\Engineering_Study\references\validation\toolchain\573-h1-hidden-type-negative.stderr.txt
rmdir /s /q "%BUILD%" 2>nul
"%CMAKE%" -S "%H1%" -B "%BUILD%" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 10
"%CMAKE%" --build "%BUILD%" --target H1_modules_reference H1_visibility_boundary H1_global_private_fragment --verbose
if errorlevel 1 exit /b 11
ctest --test-dir "%BUILD%" -R "H1_(modules|visibility|global_private)" --output-on-failure
if errorlevel 1 exit /b 12
"%CMAKE%" --build "%BUILD%" --target H1_hidden_type_negative --verbose > "%NEGOUT%" 2> "%NEGERR%"
if not errorlevel 1 exit /b 13
findstr /c:"C2065" "%NEGOUT%" >nul
if errorlevel 1 exit /b 14
exit /b 0
