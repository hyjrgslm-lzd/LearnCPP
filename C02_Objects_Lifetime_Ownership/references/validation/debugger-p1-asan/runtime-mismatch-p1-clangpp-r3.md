# P1 clang++ ASan rethrow diagnosis

Date: 2026-09-09
Scope: diagnostic sources and `build/c02-asan-debugger` only. No P1 source edits were made in this diagnostic pass.

## Question

Check whether the earlier P1 Clang++/Ninja ASan rethrow failure came from linking MD-compiled objects as the default MT ASan runtime shape.

## Erratum

This report does not prove that the original P1 failure was caused by a stale artifact. The original P1 failure and the independent `rethrow_only` failure were captured before the RAII rollback header update. The fact that the old object is older than the current header only proves that the current RAII header cannot affect that already-built object until the translation unit is rebuilt.

This report proves a narrower point: the original Ninja link shape was already the MD/dynamic ASan thunk shape with the SEH interceptor. It rules out the specific MD-object/MT-ASan-link-mismatch hypothesis for the P1 Clang++/Ninja failure path.

## Toolchain

`D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin\clang++.exe --version`:

```text
clang version 22.1.3 (https://github.com/llvm/llvm-project e9846648fd6183ee6d8cbdb4502213fcf902a211)
Target: x86_64-pc-windows-msvc
Thread model: posix
InstalledDir: D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin
```

## Link driver evidence

Bare `clang++ -fsanitize=address`:

- `runtime-mismatch-p1-clangpp-default-link-driver-r1.json`
- Link uses `-defaultlib:libcmt`
- Link uses `clang_rt.asan_dynamic-x86_64.lib`
- Link uses `clang_rt.asan_static_runtime_thunk-x86_64.lib`
- No `-include:__asan_seh_interceptor`

Bare `clang++ -fsanitize=address -fms-runtime-lib=dll`:

- `runtime-mismatch-p1-clangpp-dll-link-driver-r1.json`
- Compile cc1 gets `-D_MT`, `-D_DLL`, and `--dependent-lib=msvcrt`
- Link still uses `-defaultlib:libcmt`
- Link still uses `clang_rt.asan_static_runtime_thunk-x86_64.lib`
- No `-include:__asan_seh_interceptor`

CMake/Ninja-like `clang++` command with `-D_DLL -D_MT -Xclang --dependent-lib=msvcrt`:

- `runtime-mismatch-cmake-like-default-link-driver-r1.json`
- Link uses `clang_rt.asan_dynamic-x86_64.lib`
- Link uses `-include:__asan_seh_interceptor`
- Link uses `clang_rt.asan_dynamic_runtime_thunk-x86_64.lib`

CMake/Ninja-like plus `-fms-runtime-lib=dll`:

- `runtime-mismatch-cmake-like-dll-link-driver-r1.json`
- Same dynamic ASan thunk/interceptor shape as the CMake/Ninja-like default.

Conclusion: for this `clang++` driver, `-fms-runtime-lib=dll` alone is not the closed fix. The CMake/Ninja-style `_DLL` plus `msvcrt` object/link context is what makes the driver choose the dynamic ASan thunk/interceptor shape.

## Original Ninja evidence

`ninja -C build\c02-p1-author-asan -t commands P1_object_buffer_reference.exe` shows:

```text
clang++.exe ... -D_DLL -D_MT -Xclang --dependent-lib=msvcrt ... -fsanitize=address ... -c run_checks.cpp
clang++.exe -nostartfiles -nostdlib ... -D_DLL -D_MT -Xclang --dependent-lib=msvcrt ... -fsanitize=address ... run_checks.cpp.obj -o P1_object_buffer_reference.exe ...
```

The original object directives show MD dynamic CRT:

```text
/DEFAULTLIB:msvcrt.lib
/FAILIFMISMATCH:RuntimeLibrary=MD_DynamicRelease
/DEFAULTLIB:msvcprt.lib
/DEFAULTLIB:stl_asan.lib
```

Original Ninja link-only `-###`:

- `runtime-mismatch-original-ninja-link-driver-r1.json`
- Link uses `clang_rt.asan_dynamic-x86_64.lib`
- Link uses `-include:__asan_seh_interceptor`
- Link uses `clang_rt.asan_dynamic_runtime_thunk-x86_64.lib`

Adding `-fms-runtime-lib=dll` to the original Ninja link-only probe did not change that shape:

- `runtime-mismatch-original-ninja-link-driver-dllflag-r1.json`

Conclusion: the original Ninja link driver did not link an MD object as the static/MT ASan thunk shape. This is separate from the L14 MSVC/LLVM DLL mismatch case; do not merge those two diagnoses.

## Earlier throw/rethrow observations

Before the RAII header update, two diagnostic observations were recorded:

- `throw-only-asan-o1-run-r1.json`: ordinary cross-function `throw`/`catch` passed under ASan.
- `rethrow-only-asan-o1-run-r1.json`: independent `catch (...) { throw; }` failed under ASan.

Boundary: these observations show that the failure mode was related to rethrow/EH handling under the tested ASan configuration, not ordinary exception handling. They do not identify the lower-level component as compiler, runtime, linker thunk, or library bug by themselves.

## Rebuild evidence

Earlier executable evidence:

- `p1-reference-direct-r2.json`: FAIL at `object_buffer.hpp:166` in `grow_and_append`.

Relinking the pre-RAII object with the current dynamic ASan link shape still fails:

- Input object: `build\c02-p1-author-asan\CMakeFiles\P1_object_buffer_reference.dir\checks\run_checks.cpp.obj`
- Object size: 298772 bytes
- Object LastWriteTime: 2026-09-08 22:59:32
- Object SHA256: `57EDFD97C5AEFFD304070FF040D572F125EE515E3453446AB8D07A2EEE49CCD9`
- Result: `runtime-mismatch-p1-relinked-original-obj-run-r3.json` FAIL at `object_buffer.hpp:166`

Recompiling with the current RAII header and the original Ninja compile command, then linking with the original Ninja link command, passes:

- Recompiled object: `build\c02-asan-debugger\run_checks_recompiled_r3.obj`
- Object size: 324210 bytes
- Object LastWriteTime: 2026-09-09 10:43:34
- Object SHA256: `633DC9A00E93FCAC41561904E9208B69250103C997244DB8AA15C119B7370866`
- Result: `runtime-mismatch-p1-recompiled-obj-run-r3.json` PASS with `P1_object_buffer_contract OK`

The current header is newer than the failing object:

- `Core_Study\exercises\P1_object_buffer\src\reference\object_buffer.hpp`
- Header LastWriteTime: 2026-09-08 23:26:22
- Header SHA256: `11031B05BAF698837601183CD021AB24ED11014639E2F49053F0ACDBA9EE0861`
- Current code uses RAII rollback and no longer has the old explicit cleanup `throw;` at the previous failure shape.

## Result

The current build flags are not the suspected MT/static-thunk failure shape. For the current RAII header, a rebuild is required for the new header body to take effect, and the rebuilt diagnostic object passes under the same Ninja-style dynamic ASan link shape.

The lower-level root cause of the pre-RAII explicit-rethrow failure is not determined by this report. Keep that evidence separate from the later L14 MSVC/LLVM runtime DLL mismatch, which is a different loader/export compatibility problem.
