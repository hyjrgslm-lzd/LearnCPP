# L14 ASan safe default runtime diagnostics

## Commands just executed
### L14-asan-safe-default-configure.json
command: cmake -S Core_Study/exercises/L14_ub -B build/c02-storage-verifier-r1/L14-asan-safe-default -G Visual Studio 18 2026 -A x64 -DCORE_STUDY_ENABLE_ASAN=ON
exit_code: 0
verdict: PASS
stdout excerpt:
-- The CXX compiler identification is MSVC 19.51.36256.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: D:/VisualStudio2026/Installed/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done (2.5s)
-- Generating done (0.1s)
-- Build files have been written to: F:/CPPTrain/LearnCPP/build/c02-storage-verifier-r1/L14-asan-safe-default


### L14-asan-safe-default-build-debug.json
command: cmake --build build/c02-storage-verifier-r1/L14-asan-safe-default --config Debug --clean-first
exit_code: 0
verdict: PASS
stdout excerpt:
适用于 .NET Framework MSBuild 版本 18.9.1+a81b43525

适用于 .NET Framework MSBuild 版本 18.9.1+a81b43525

  1>Checking Build System
  Building Custom Rule F:/CPPTrain/LearnCPP/Core_Study/exercises/L14_ub/CMakeLists.txt
  observation_check.cpp
  L14_ub_observation.vcxproj -> F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\Debug\L14_ub_observation.exe
  Building Custom Rule F:/CPPTrain/LearnCPP/Core_Study/exercises/L14_ub/CMakeLists.txt


### L14-asan-safe-default-ctest-debug.json
command: ctest --test-dir build/c02-storage-verifier-r1/L14-asan-safe-default -C Debug --output-on-failure
exit_code: 8
verdict: FAIL
stdout excerpt:
Test project F:/CPPTrain/LearnCPP/build/c02-storage-verifier-r1/L14-asan-safe-default
    Start 1: L14_ub_observation
1/1 Test #1: L14_ub_observation ...............Exit code 0xc0000135***Exception:   8.30 sec


0% tests passed, 1 tests failed out of 1

Label Time Summary:
observation    =   8.30 sec*proc (1 test)

Total Test time (real) =   8.31 sec

The following tests FAILED:
	  1 - L14_ub_observation (Exit code 0xc0000135)         observation

stderr excerpt:
Errors while running CTest


### run-l14-asan-safe-default-ctest-debug-with-path.cmd
@echo off
set "PATH=D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin;D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows;%PATH%"
set "ASAN_OPTIONS=halt_on_error=1:exitcode=1"
ctest --test-dir build/c02-storage-verifier-r1/L14-asan-safe-default -C Debug --output-on-failure

### L14-asan-safe-default-ctest-debug-with-path.json
command: cmd /c F:\CPPTrain\LearnCPP\Core_Study\references\validation\reviews\c02-storage-verifier-evidence-r1\run-l14-asan-safe-default-ctest-debug-with-path.cmd
exit_code: 8
verdict: FAIL
stdout excerpt:
Test project F:/CPPTrain/LearnCPP/build/c02-storage-verifier-r1/L14-asan-safe-default
    Start 1: L14_ub_observation
1/1 Test #1: L14_ub_observation ...............Exit code 0xc0000139***Exception:  16.47 sec


0% tests passed, 1 tests failed out of 1

Label Time Summary:
observation    =  16.47 sec*proc (1 test)

Total Test time (real) =  16.49 sec

The following tests FAILED:
	  1 - L14_ub_observation (Exit code 0xc0000139)         observation

stderr excerpt:
Errors while running CTest


## Build artifacts
exe exists: True
exe path: F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\Debug\L14_ub_observation.exe
exe sha256: F0CE112DDFC2C53BBA605676803A6BF8F8A1A4FA7935B0D508803FF877E09442

## CMake cache compiler/toolset lines
CMAKE_CXX_FLAGS:STRING=/DWIN32 /D_WINDOWS /EHsc
CMAKE_CXX_FLAGS_DEBUG:STRING=/Ob0 /Od /RTC1
CMAKE_CXX_FLAGS_MINSIZEREL:STRING=/O1 /Ob1 /DNDEBUG
CMAKE_CXX_FLAGS_RELEASE:STRING=/O2 /Ob2 /DNDEBUG
CMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=/O2 /Ob1 /DNDEBUG
CMAKE_EXE_LINKER_FLAGS:STRING=/machine:x64
CMAKE_EXE_LINKER_FLAGS_DEBUG:STRING=/debug /INCREMENTAL
CMAKE_EXE_LINKER_FLAGS_MINSIZEREL:STRING=/INCREMENTAL:NO
CMAKE_EXE_LINKER_FLAGS_RELEASE:STRING=/INCREMENTAL:NO
CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO:STRING=/debug /INCREMENTAL
//ADVANCED property for variable: CMAKE_CXX_FLAGS
CMAKE_CXX_FLAGS-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_CXX_FLAGS_DEBUG
CMAKE_CXX_FLAGS_DEBUG-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_CXX_FLAGS_MINSIZEREL
CMAKE_CXX_FLAGS_MINSIZEREL-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_CXX_FLAGS_RELEASE
CMAKE_CXX_FLAGS_RELEASE-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_CXX_FLAGS_RELWITHDEBINFO
CMAKE_CXX_FLAGS_RELWITHDEBINFO-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_EXE_LINKER_FLAGS
CMAKE_EXE_LINKER_FLAGS-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_EXE_LINKER_FLAGS_DEBUG
CMAKE_EXE_LINKER_FLAGS_DEBUG-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_EXE_LINKER_FLAGS_MINSIZEREL
CMAKE_EXE_LINKER_FLAGS_MINSIZEREL-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_EXE_LINKER_FLAGS_RELEASE
CMAKE_EXE_LINKER_FLAGS_RELEASE-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO
CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO-ADVANCED:INTERNAL=1
CMAKE_GENERATOR:INTERNAL=Visual Studio 18 2026
CMAKE_GENERATOR_INSTANCE:INTERNAL=D:/VisualStudio2026/Installed
CMAKE_GENERATOR_PLATFORM:INTERNAL=x64
CMAKE_GENERATOR_TOOLSET:INTERNAL=

## vcxproj sanitizer/compiler/runtime lines
27: <WindowsTargetPlatformVersion>10.0.26100.0</WindowsTargetPlatformVersion>
36: <PlatformToolset>v145</PlatformToolset>
37: <EnableAsan>true</EnableAsan>
42: <PlatformToolset>v145</PlatformToolset>
43: <EnableAsan>true</EnableAsan>
48: <PlatformToolset>v145</PlatformToolset>
49: <EnableAsan>true</EnableAsan>
54: <PlatformToolset>v145</PlatformToolset>
55: <EnableAsan>true</EnableAsan>
92: <ClCompile>
94: <AdditionalOptions>%(AdditionalOptions) /utf-8 /Zc:__cplusplus /fsanitize=address</AdditionalOptions>
110: <RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>
118: </ClCompile>
132: <AdditionalDependencies>kernel32.lib;user32.lib;gdi32.lib;winspool.lib;shell32.lib;ole32.lib;oleaut32.lib;uuid.lib;comdlg32.lib;advapi32.lib</AdditionalDependencies>
133: <AdditionalLibraryDirectories>%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
134: <AdditionalOptions>%(AdditionalOptions) /machine:x64</AdditionalOptions>
150: <ClCompile>
152: <AdditionalOptions>%(AdditionalOptions) /utf-8 /Zc:__cplusplus /fsanitize=address</AdditionalOptions>
168: <RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>
176: </ClCompile>
190: <AdditionalDependencies>kernel32.lib;user32.lib;gdi32.lib;winspool.lib;shell32.lib;ole32.lib;oleaut32.lib;uuid.lib;comdlg32.lib;advapi32.lib</AdditionalDependencies>
191: <AdditionalLibraryDirectories>%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
192: <AdditionalOptions>%(AdditionalOptions) /machine:x64</AdditionalOptions>
208: <ClCompile>
210: <AdditionalOptions>%(AdditionalOptions) /utf-8 /Zc:__cplusplus /fsanitize=address</AdditionalOptions>
226: <RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>
234: </ClCompile>
248: <AdditionalDependencies>kernel32.lib;user32.lib;gdi32.lib;winspool.lib;shell32.lib;ole32.lib;oleaut32.lib;uuid.lib;comdlg32.lib;advapi32.lib</AdditionalDependencies>
249: <AdditionalLibraryDirectories>%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
250: <AdditionalOptions>%(AdditionalOptions) /machine:x64</AdditionalOptions>
266: <ClCompile>
268: <AdditionalOptions>%(AdditionalOptions) /utf-8 /Zc:__cplusplus /fsanitize=address</AdditionalOptions>
284: <RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>
292: </ClCompile>
306: <AdditionalDependencies>kernel32.lib;user32.lib;gdi32.lib;winspool.lib;shell32.lib;ole32.lib;oleaut32.lib;uuid.lib;comdlg32.lib;advapi32.lib</AdditionalDependencies>
307: <AdditionalLibraryDirectories>%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
308: <AdditionalOptions>%(AdditionalOptions) /machine:x64</AdditionalOptions>
381: <ClCompile Include="F:\CPPTrain\LearnCPP\Core_Study\exercises\L14_ub\checks\observation_check.cpp" />

## tlog command/import clues
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\CMakeFiles\4.2.3\CompilerIdCXX\Debug\CompilerIdCXX.tlog\CL.command.1.tlog
^F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\4.2.3\COMPILERIDCXX\CMAKECXXCOMPILERID.CPP
/c /nologo /W0 /WX- /diagnostics:column /Od /D _MBCS /Gm- /EHsc /RTC1 /MDd /GS /fp:precise /Zc:wchar_t /Zc:forScope /Zc:inline /Fo"DEBUG\\" /Fd"DEBUG\VC145.PDB" /external:W0 /Gd /TP /FC F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\4.2.3\COMPILERIDCXX\CMAKECXXCOMPILERID.CPP
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\CMakeFiles\4.2.3\CompilerIdCXX\Debug\CompilerIdCXX.tlog\Cl.items.tlog
F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\CMakeFiles\4.2.3\CompilerIdCXX\CMakeCXXCompilerId.cpp;F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\CMakeFiles\4.2.3\CompilerIdCXX\Debug\CMakeCXXCompilerId.obj
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\CMakeFiles\4.2.3\CompilerIdCXX\Debug\CompilerIdCXX.tlog\CL.read.1.tlog
^F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\4.2.3\COMPILERIDCXX\CMAKECXXCOMPILERID.CPP
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\CMakeFiles\4.2.3\CompilerIdCXX\Debug\CompilerIdCXX.tlog\CL.write.1.tlog
^F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\4.2.3\COMPILERIDCXX\CMAKECXXCOMPILERID.CPP
F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\4.2.3\COMPILERIDCXX\DEBUG\CMAKECXXCOMPILERID.OBJ
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\CMakeFiles\4.2.3\CompilerIdCXX\Debug\CompilerIdCXX.tlog\link.command.1.tlog
^F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\4.2.3\COMPILERIDCXX\DEBUG\CMAKECXXCOMPILERID.OBJ
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\CMakeFiles\4.2.3\CompilerIdCXX\Debug\CompilerIdCXX.tlog\link.read.1.tlog
^F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\4.2.3\COMPILERIDCXX\DEBUG\CMAKECXXCOMPILERID.OBJ
F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\4.2.3\COMPILERIDCXX\DEBUG\CMAKECXXCOMPILERID.OBJ
D:\VISUALSTUDIO2026\INSTALLED\VC\TOOLS\MSVC\14.51.36231\LIB\X64\VCRUNTIMED.LIB
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\LIB\10.0.26100.0\UCRT\X64\UCRTD.LIB
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\CMakeFiles\4.2.3\CompilerIdCXX\Debug\CompilerIdCXX.tlog\link.secondary.1.tlog
^F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\4.2.3\COMPILERIDCXX\DEBUG\CMAKECXXCOMPILERID.OBJ
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\CMakeFiles\4.2.3\CompilerIdCXX\Debug\CompilerIdCXX.tlog\link.write.1.tlog
^F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\4.2.3\COMPILERIDCXX\DEBUG\CMAKECXXCOMPILERID.OBJ
F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\4.2.3\COMPILERIDCXX\COMPILERIDCXX.EXE
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\L14_ub_observation.dir\Debug\L14_ub_o.66F2A3EA.tlog\CL.command.1.tlog
/c /IF:\CPPTRAIN\LEARNCPP\CORE_STUDY\EXERCISES\CMAKE\..\..\..\ENGINEERING_STUDY\EXERCISES\INCLUDE /Zi /nologo /W4 /WX- /diagnostics:column /fsanitize=address /Od /Ob0 /D _MBCS /D WIN32 /D _WINDOWS /D "CMAKE_INTDIR=\"Debug\"" /EHsc /RTC1 /MDd /std:c++latest /permissive- /Fo"L14_UB_OBSERVATION.DIR\DEBUG\\" /Fd"L14_UB_OBSERVATION.DIR\DEBUG\VC145.PDB" /external:W4 /TP  /utf-8 /Zc:__cplusplus /fsanitize=address F:\CPPTRAIN\LEARNCPP\CORE_STUDY\EXERCISES\L14_UB\CHECKS\OBSERVATION_CHECK.CPP
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\L14_ub_observation.dir\Debug\L14_ub_o.66F2A3EA.tlog\Cl.items.tlog
F:\CPPTrain\LearnCPP\Core_Study\exercises\L14_ub\checks\observation_check.cpp;F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\L14_ub_observation.dir\Debug\observation_check.obj
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\L14_ub_observation.dir\Debug\L14_ub_o.66F2A3EA.tlog\CL.read.1.tlog
D:\VISUALSTUDIO2026\INSTALLED\VC\TOOLS\MSVC\14.51.36231\INCLUDE\VCRUNTIME.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\MATH.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_MATH.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\STDLIB.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_MALLOC.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_SEARCH.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\STDDEF.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_WSTDLIB.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\FLOAT.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CRTDBG.H
D:\VISUALSTUDIO2026\INSTALLED\VC\TOOLS\MSVC\14.51.36231\INCLUDE\VCRUNTIME_NEW_DEBUG.H
D:\VISUALSTUDIO2026\INSTALLED\VC\TOOLS\MSVC\14.51.36231\INCLUDE\VCRUNTIME_NEW.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\STDIO.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_WSTDIO.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_STDIO_CONFIG.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\STRING.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_MEMORY.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_MEMCPY_S.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\ERRNO.H
D:\VISUALSTUDIO2026\INSTALLED\VC\TOOLS\MSVC\14.51.36231\INCLUDE\VCRUNTIME_STRING.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_WSTRING.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\WCHAR.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_WCONIO.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_WCTYPE.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_WDIRECT.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_WIO.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_SHARE.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_WPROCESS.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_WTIME.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\SYS\STAT.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\SYS\TYPES.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\MALLOC.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\SHARE.H
D:\VISUALSTUDIO2026\INSTALLED\VC\TOOLS\MSVC\14.51.36231\INCLUDE\VCRUNTIME_EXCEPTION.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CORECRT_TERMINATE.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\TIME.H
D:\VISUALSTUDIO2026\INSTALLED\VC\TOOLS\MSVC\14.51.36231\INCLUDE\VCRUNTIME_TYPEINFO.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\CTYPE.H
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\INCLUDE\10.0.26100.0\UCRT\LOCALE.H
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\L14_ub_observation.dir\Debug\L14_ub_o.66F2A3EA.tlog\CL.write.1.tlog
F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\L14_UB_OBSERVATION.DIR\DEBUG\VC145.PDB
F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\L14_UB_OBSERVATION.DIR\DEBUG\OBSERVATION_CHECK.OBJ
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\L14_ub_observation.dir\Debug\L14_ub_o.66F2A3EA.tlog\CustomBuild.command.1.tlog
D:\cmake\install\bin\cmake.exe -SF:/CPPTrain/LearnCPP/Core_Study/exercises/L14_ub -BF:/CPPTrain/LearnCPP/build/c02-storage-verifier-r1/L14-asan-safe-default --check-stamp-file F:/CPPTrain/LearnCPP/build/c02-storage-verifier-r1/L14-asan-safe-default/CMakeFiles/generate.stamp
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\L14_ub_observation.dir\Debug\L14_ub_o.66F2A3EA.tlog\CustomBuild.read.1.tlog
F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\4.2.3\CMAKECXXCOMPILER.CMAKE
F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\4.2.3\CMAKERCCOMPILER.CMAKE
F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\4.2.3\CMAKESYSTEM.CMAKE
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\L14_ub_observation.dir\Debug\L14_ub_o.66F2A3EA.tlog\CustomBuild.write.1.tlog
F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\CMAKEFILES\GENERATE.STAMP
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\L14_ub_observation.dir\Debug\L14_ub_o.66F2A3EA.tlog\link.command.1.tlog
^F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\L14_UB_OBSERVATION.DIR\DEBUG\OBSERVATION_CHECK.OBJ
/OUT:"F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\DEBUG\L14_UB_OBSERVATION.EXE" /INCREMENTAL:NO /NOLOGO KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB COMDLG32.LIB ADVAPI32.LIB /MANIFEST /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /manifest:embed /DEBUG /PDB:"F:/CPPTRAIN/LEARNCPP/BUILD/C02-STORAGE-VERIFIER-R1/L14-ASAN-SAFE-DEFAULT/DEBUG/L14_UB_OBSERVATION.PDB" /SUBSYSTEM:CONSOLE /TLBID:1 /IMPLIB:"F:/CPPTRAIN/LEARNCPP/BUILD/C02-STORAGE-VERIFIER-R1/L14-ASAN-SAFE-DEFAULT/DEBUG/L14_UB_OBSERVATION.LIB" /MACHINE:X64  /machine:x64 L14_UB_OBSERVATION.DIR\DEBUG\OBSERVATION_CHECK.OBJ
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\L14_ub_observation.dir\Debug\L14_ub_o.66F2A3EA.tlog\link.read.1.tlog
^F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\L14_UB_OBSERVATION.DIR\DEBUG\OBSERVATION_CHECK.OBJ
F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\L14_UB_OBSERVATION.DIR\DEBUG\OBSERVATION_CHECK.OBJ
D:\VISUALSTUDIO2026\INSTALLED\VC\TOOLS\MSVC\14.51.36231\LIB\X64\CLANG_RT.ASAN_DYNAMIC-X86_64.LIB
D:\VISUALSTUDIO2026\INSTALLED\VC\TOOLS\MSVC\14.51.36231\LIB\X64\CLANG_RT.ASAN_DYNAMIC_RUNTIME_THUNK-X86_64.LIB
D:\VISUALSTUDIO2026\INSTALLED\VC\TOOLS\MSVC\14.51.36231\LIB\X64\STL_ASAN.LIB
D:\VISUALSTUDIO2026\INSTALLED\VC\TOOLS\MSVC\14.51.36231\LIB\X64\VCASAND.LIB
D:\VISUALSTUDIO2026\INSTALLED\VC\TOOLS\MSVC\14.51.36231\LIB\X64\VCRUNTIMED.LIB
C:\PROGRAM FILES (X86)\WINDOWS KITS\10\LIB\10.0.26100.0\UCRT\X64\UCRTD.LIB
F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\L14_UB_OBSERVATION.DIR\DEBUG\VC145.PDB
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\L14_ub_observation.dir\Debug\L14_ub_o.66F2A3EA.tlog\link.secondary.1.tlog
^F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\L14_UB_OBSERVATION.DIR\DEBUG\OBSERVATION_CHECK.OBJ
### F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\L14_ub_observation.dir\Debug\L14_ub_o.66F2A3EA.tlog\link.write.1.tlog
^F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\L14_UB_OBSERVATION.DIR\DEBUG\OBSERVATION_CHECK.OBJ
F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\DEBUG\L14_UB_OBSERVATION.EXE
F:\CPPTRAIN\LEARNCPP\BUILD\C02-STORAGE-VERIFIER-R1\L14-ASAN-SAFE-DEFAULT\DEBUG\L14_UB_OBSERVATION.PDB

## PATH used by with-path cmd
D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin;D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows;%PATH%
ASAN_OPTIONS=halt_on_error=1:exitcode=1
