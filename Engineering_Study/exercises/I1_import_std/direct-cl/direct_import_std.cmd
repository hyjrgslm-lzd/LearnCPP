@echo off
setlocal
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64
if errorlevel 1 exit /b 1
set OUT=%~dp0out
mkdir "%OUT%" 2>nul
pushd "%OUT%"
cl /nologo /std:c++latest /EHsc /c /interface "D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\modules\std.ixx" /ifcOutput std.ifc /Fostd.obj
if errorlevel 1 exit /b 2
cl /nologo /std:c++latest /EHsc /c "%~dp0..\reference\main.cpp" /reference std=std.ifc /Fomain.obj
if errorlevel 1 exit /b 3
link /nologo main.obj std.obj /out:import_std_direct.exe
if errorlevel 1 exit /b 4
import_std_direct.exe
exit /b %errorlevel%
