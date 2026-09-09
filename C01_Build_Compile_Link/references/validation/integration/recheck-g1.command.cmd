@echo off
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1
set VSLANG=1033
cmake --build "F:\CPPTrain\LearnCPP\Engineering_Study\exercises\build\final-matrix-r3\verify-core" --config Release --target G1_diagnostics_reference G1_diagnostics_student > "F:\CPPTrain\LearnCPP\Engineering_Study\references\validation\integration\g1-final-Release-build.txt" 2>&1
if errorlevel 1 exit /b 1
ctest --test-dir "F:\CPPTrain\LearnCPP\Engineering_Study\exercises\build\final-matrix-r3\verify-core" -C Release -R G1_diagnostics --output-on-failure --output-junit "F:\CPPTrain\LearnCPP\Engineering_Study\references\validation\integration\g1-final-Release.xml"
if errorlevel 1 exit /b 1
cmake --build "F:\CPPTrain\LearnCPP\Engineering_Study\exercises\build\final-matrix-r3\verify-debug" --config Debug --target G1_diagnostics_reference G1_diagnostics_student > "F:\CPPTrain\LearnCPP\Engineering_Study\references\validation\integration\g1-final-Debug-build.txt" 2>&1
if errorlevel 1 exit /b 1
ctest --test-dir "F:\CPPTrain\LearnCPP\Engineering_Study\exercises\build\final-matrix-r3\verify-debug" -C Debug -R G1_diagnostics --output-on-failure --output-junit "F:\CPPTrain\LearnCPP\Engineering_Study\references\validation\integration\g1-final-Debug.xml"
exit /b %errorlevel%
