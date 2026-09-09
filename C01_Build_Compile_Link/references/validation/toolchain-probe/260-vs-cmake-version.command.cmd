@echo off
call "D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
"D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --version
"D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" --version
