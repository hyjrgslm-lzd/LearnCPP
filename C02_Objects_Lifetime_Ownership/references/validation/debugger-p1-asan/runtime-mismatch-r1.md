# L14 MSVC ASan runtime mismatch static check

Date: 2026-09-09
Scope: static PE/project inspection only. The target executable was not run.

## Target

- EXE: `F:\CPPTrain\LearnCPP\build\c02-storage-verifier-r1\L14-asan-safe-default\Debug\L14_ub_observation.exe`
- Size: 87040 bytes
- LastWriteTime: 2026-09-09 10:19:30
- SHA256: `F0CE112DDFC2C53BBA605676803A6BF8F8A1A4FA7935B0D508803FF877E09442`

## Project evidence

`build\c02-storage-verifier-r1\L14-asan-safe-default\L14_ub_observation.vcxproj` shows:

- `PlatformToolset` = `v145`
- `EnableAsan` = `true`
- Debug `AdditionalOptions` includes `/fsanitize=address`
- Debug `RuntimeLibrary` = `MultiThreadedDebugDLL`

This is an MSVC v145 Debug ASan build, not a Clang++/LLVM runtime build.

## Import evidence

`dumpbin /imports` and `llvm-readobj --coff-imports` both show the EXE imports `clang_rt.asan_dynamic-x86_64.dll` by name and requires:

- `__sanitizer_cov_8bit_counters_init__dll`
- `__sanitizer_cov_8bit_counters_cleanup__dll`
- `__sanitizer_cov_pcs_cleanup__dll`
- `__sanitizer_cov_trace_pc_guard_cleanup__dll`
- other `__asan_*` and `__sanitizer_cov_*` symbols

The output directory does not currently contain `clang_rt.asan_dynamic-x86_64.dll`, so Windows loader resolution depends on the DLL search path.

## Runtime candidates

MSVC 14.51 x64 ASan runtime:

- Path: `D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\clang_rt.asan_dynamic-x86_64.dll`
- Size: 1881912 bytes
- LastWriteTime: 2026-09-02 11:53:17
- ProductVersion: `14.51.36256.0`
- FileVersion: `19.51.36256.0`
- SHA256: `7F901A239979C49B36A3B18ECD1843F551F94B5879585BDED41A650C5B71F35F`
- Exports required cleanup symbols, including `__sanitizer_cov_8bit_counters_cleanup__dll`.

LLVM 22 ASan runtime:

- Path: `D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows\clang_rt.asan_dynamic-x86_64.dll`
- Size: 539184 bytes
- LastWriteTime: 2026-09-02 11:54:00
- ProductVersion: empty
- FileVersion: empty
- SHA256: `0C77CDD870DBD719A3475CFB02507CC6B56D6DFA5D46FC960177CFE1A1298FF7`
- Exports `__sanitizer_cov_8bit_counters_init__dll`.
- Does not export `__sanitizer_cov_8bit_counters_cleanup__dll`.

## Conclusion

The reported loader sequence matches a DLL resolution problem:

1. Missing `clang_rt.asan_dynamic-x86_64.dll`: no ASan runtime was found.
2. Missing `__sanitizer_cov_8bit_counters_cleanup__dll`: a same-name but incompatible LLVM 22 runtime was found.

Use the MSVC 14.51 x64 runtime for this target:

`D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\clang_rt.asan_dynamic-x86_64.dll`

Do not use the LLVM 22 runtime for this MSVC v145 `/fsanitize=address` Debug executable.
