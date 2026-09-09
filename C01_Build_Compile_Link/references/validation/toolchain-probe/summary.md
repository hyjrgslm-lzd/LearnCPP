# C01 toolchain probe summary

Date: 2026-09-08.

Scope: local probe only. This is not a completed C01 lesson unit.

## Environment observed

- CMake: 4.2.3 (`00-tool-versions.*`)
- Ninja: 1.12.0 (`00-tool-versions.*`)
- MSVC compiler: 19.51.36256, toolset path `D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231` (`00-tool-versions.*`)
- Clang: `D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin\clang++.exe`
- Clang sanitizer runtime DLL found at `D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows\clang_rt.asan_dynamic-x86_64.dll`

## Results

| Probe | Result | Evidence |
|---|---:|---|
| MSVC named module, direct `cl` interface/main/link/run | PASS | `50-direct-msvc-named-module-interface.meta.json` through `53-direct-msvc-named-module-run.meta.json`, all exit `0` |
| MSVC `import std`, direct `std.ixx` build + consumer link/run | PASS | `60-direct-msvc-std-module-interface.meta.json` through `63-direct-msvc-import-std-run.meta.json`, all exit `0`; run stdout is `10` |
| CMake 4.2.3 + VS Ninja named module configure/build/run, normal execution surface | PASS | `400-normal-cmake-chain.meta.json`: exit `0`; stdout shows ABI detection done, dyndep, module compile, link, and run |
| CMake 4.2.3 + VS Ninja `import std`, normal execution surface | PASS | `400-normal-cmake-chain.meta.json`: exit `0`; stdout shows `std.ixx`/`std.compat.ixx` compiled and run stdout `10`; `normal-import-std/CMakeFiles/*/CXXModules.json` records `std.ifc` and `std.compat.ifc` |
| CMake module `FILE_SET CXX_MODULES` install/export + independent consumer, normal execution surface | PASS | `400-normal-cmake-chain.meta.json`: exit `0`; install wrote `cxx-modules/arithmetic_package.ixx`, exported CMake module files, consumer recompiled BMI from installed module source, linked `arithmetic_module.lib`, and ran |
| CMake configure in restricted/log-runner surface | FAIL/TIMEOUT | `10-msvc-named-modules-configure.meta.json` and `20-msvc-import-std-configure.meta.json`: earlier runs timed out during compiler ABI detection; later direct normal execution proves this is not module/import-std unsupported |
| Clang ASan build | PASS | `70-direct-clang-asan-build.meta.json`: exit `0` |
| Clang ASan run without runtime path | FAIL/RUNTIME | `71-direct-clang-asan-run.meta.json`: exit `-1073741511`, stderr empty |
| Clang ASan run with sanitizer runtime path | PASS | `80-direct-clang-asan-run-with-runtime-path.meta.json`: exit `0` |
| Clang ASan detection report | PASS | `81-direct-clang-asan-detect-build.meta.json`: exit `0`; `82-direct-clang-asan-detect-run-with-runtime-path.meta.json`: exit `1` with `AddressSanitizer: heap-buffer-overflow` |
| Clang libFuzzer build | PASS | `72-direct-clang-libfuzzer-build.meta.json`: exit `0` |
| Clang libFuzzer run without runtime path | FAIL/RUNTIME | `73-direct-clang-libfuzzer-run.meta.json`: exit `-1073741511`, stderr empty |
| Clang libFuzzer run with sanitizer runtime path | PASS | `83-direct-clang-libfuzzer-run-with-runtime-path.meta.json`: exit `0`, stderr includes `Done 8 runs` |
| Clang `-ftime-trace` | PASS | `74-direct-clang-ftime-trace-build.meta.json`: exit `0`; `75-direct-clang-ftime-trace-check.stdout.txt` lists `trace_probe.exe-trace.json` |

## Recommendation for C01 F-I authoring

Use direct MSVC module commands as the known-good minimal path for explaining module mechanics:

- module interface: `cl /std:c++latest /EHsc /c /interface arithmetic.ixx /ifcOutput arithmetic.ifc`
- consumer: `cl /std:c++latest /EHsc /c main.cpp /reference arithmetic=arithmetic.ifc`
- link with both objects.

For `import std`, this environment can build and consume MSVC's installed `std.ixx` directly and through CMake 4.2.3 + VS Ninja when the normal local execution surface is used. The probe CMakeLists keeps the official experimental gate before `project()`, checks `CMAKE_CXX_COMPILER_IMPORT_STD`, and enables target `CXX_MODULE_STD`.

For Clang sanitizer/fuzzer lessons, add the Clang runtime directory to the child process `PATH` before running sanitizer/fuzzer binaries. Build success alone is not enough on Windows; without the runtime DLL path the process failed before producing sanitizer/fuzzer output.

For module package/install lessons, the verified minimal CMake shape is `FILE_SET CXX_MODULES` plus `install(EXPORT ... CXX_MODULES_DIRECTORY ...)`. The installed package distributes module interface source and CMake metadata, not a compiler-portable BMI. The independent consumer rebuilt its own BMI from the installed module source and linked the installed library.

The earlier timeout records remain useful as an execution-surface boundary: restricted runner processes could hang in default ABI try_compile, while the normal escalated local process completed the same CMake/MSVC/Ninja class of work. Do not write an unproven lower-level root cause for the timeout.
