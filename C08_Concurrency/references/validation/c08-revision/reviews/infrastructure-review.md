# C08 infrastructure review 2026-09-10

Review role: non-author infrastructure review.

Review method: solo non-author pass, because the task explicitly disallowed recursive delegation. Writer/reviewer separation was preserved by editing only this review report and not editing the reviewed files.

Reviewed files:

- `C08_Concurrency/exercises/cmake/Sanitizers.cmake`
  - SHA256: `97741F3902E4A68EE2283E4975CDAFEB5525501F5822F9D0E923BC09A21B2B1E`
- `C08_Concurrency/exercises/cmake/StudySetup.cmake`
  - SHA256: `6282228300EE4DD6616F91460A526B9BE91EEFEF5ECB83D07D52A9DBCE022C1E`
- `C08_Concurrency/exercises/CMakePresets.json`
  - SHA256: `743031C9C710BA54E7D2B63DD422C22719624E58AE74778D30FF39B9A4ABAE17`
- `C08_Concurrency/exercises/tools/run_benchmarks.py`
  - SHA256: `D312DDF283D3E6F21D613C33F43FF6841C7C5277459F38B6B3EC07D60B0840AC`
- `C08_Concurrency/exercises/runtime_tests/CMakeLists.txt`
  - SHA256: `D4A48B2F46FCF6C5B2E7C041C860011EB2534154A4036BCC00A38EC5849FBDF3`
- `C08_Concurrency/exercises/benchmarks/CMakeLists.txt`
  - SHA256: `5170F77A5F048708A5904EF82113459075456C3B043A3E7A5FA94EB1795D2DB3`

Validation artifacts read:

- `C08_Concurrency/references/validation/c08-revision/asan-smoke-configure.json`
  - SHA256: `005DFFA16890509863B6820C5141DD9EFD3E69357507D0F8781B0DF13C0F1676`
- `C08_Concurrency/references/validation/c08-revision/asan-smoke-build.json`
  - SHA256: `26751E1CB72FD5756F58633663681EBAE7EDC88061E1AC204B0EA06BD8B72B7D`
- `C08_Concurrency/references/validation/c08-revision/asan-smoke-ctest.json`
  - SHA256: `FBA4772A23ED6DB5B8581F776BA5E7ADFD8CEA75864CC697E7C6C1F58F9A2A47`
- `C08_Concurrency/references/validation/c08-revision/python-version-rejection.json`
  - SHA256: `AD4F5611C767DD11FBE2D9A4D42A552E5D3C292BFB0DB0422D0E6C998E6F7C3B`

## Verdict

APPROVE.

No blocking infrastructure issue was found in the reviewed slice. The sanitizer path is default-off, sanitizer-enabled C++ targets receive compile/link instrumentation, MSVC ASan runtime copying is present and has smoke evidence, sanitizer C++ test timeout now resolves to 120 instead of being overwritten by the old per-test 30-second property, and the benchmark runner rejects Python below 3.11 before reaching APIs that require 3.11.

## Checks

1. Default behavior remains unchanged unless a sanitizer is explicitly selected.

   Evidence: `Sanitizers.cmake:2` defines `CONCURRENCY_STUDY_SANITIZER` with cache default `none`, and `Sanitizers.cmake:9` returns early for `none`. `CMakePresets.json:24` pins the base preset to `"CONCURRENCY_STUDY_SANITIZER": "none"`. `StudySetup.cmake:58` calls `cs_enable_sanitizer(${target})` for every configured target, but the default branch is no-op. This means existing default, student, benchmark, and full presets do not get instrumentation unless they override the cache value.

2. MSVC ASan compile/link/runtime path is coherent.

   Evidence: `Sanitizers.cmake:12` gates MSVC support to `address`; `Sanitizers.cmake:14` applies `/fsanitize=address /Zi`; `Sanitizers.cmake:15` disables incremental linking with `/INCREMENTAL:NO`; `Sanitizers.cmake:24` through `Sanitizers.cmake:29` locate and copy the matching `clang_rt.asan_dynamic-${arch}.dll` next to executable targets. The validation artifacts show A2 configure/build/ctest passed: `asan-smoke-configure.json` exit `0`, `asan-smoke-build.json` exit `0`, and `asan-smoke-ctest.json` exit `0` with `1/1` test passed. The smoke build output directory contains `clang_rt.asan_dynamic-x86_64.dll`, and generated `.vcxproj` files contain `/fsanitize=address` plus the ASan DLL post-build copy command.

3. Linux sanitizer presets and flags are platform-scoped.

   Evidence: `CMakePresets.json:143` through `CMakePresets.json:156` scope `linux-core` to host `Linux`; `linux-asan` and `linux-tsan` inherit it and select `clang++-18`, `RelWithDebInfo`, and sanitizer values at `CMakePresets.json:161` through `CMakePresets.json:175`. `Sanitizers.cmake:31` through `Sanitizers.cmake:38` apply `-fsanitize=address,undefined` for address mode and `-fsanitize=thread` for thread mode only on non-Windows Clang/GNU. I did not run Linux configure from this Windows review shell; this approval is based on CMake syntax/scope inspection plus the existing WSL validation baseline, not a fresh Linux build.

4. C++ test timeout is no longer silently overwritten by the old 30-second per-test property.

   Evidence: `StudySetup.cmake:18` through `StudySetup.cmake:22` sets `CS_TEST_TIMEOUT` to `30` only when sanitizer is `none`, otherwise `120`. Exercise student/observation tests use it at `StudySetup.cmake:113`; reference tests use it at `StudySetup.cmake:132`; runtime C++ tests use it at `runtime_tests/CMakeLists.txt:31`, `runtime_tests/CMakeLists.txt:39`, and `runtime_tests/CMakeLists.txt:45`; the new diagnostic CTest uses it at `benchmarks/CMakeLists.txt:20`. The sanitizer test presets also set execution timeout `120` at `CMakePresets.json:400`, `CMakePresets.json:436`, and `CMakePresets.json:448`. Python tool tests remain `TIMEOUT 30` at `runtime_tests/CMakeLists.txt:51` and `runtime_tests/CMakeLists.txt:54`; this matches the stated r2 boundary that tool tests stay 30 seconds.

5. Queue diagnostics are isolated from ordinary timed benchmarks.

   Evidence: `benchmarks/CMakeLists.txt:5` still glob-builds only `*_bench.cpp` timed drivers. `queue_diagnostics` is added as its own executable at `benchmarks/CMakeLists.txt:15`, configured at `benchmarks/CMakeLists.txt:16`, and receives `CS_QUEUE_DIAGNOSTICS=1` only at `benchmarks/CMakeLists.txt:17`. I did not inspect the author-returned driver implementation in this infrastructure review; the reviewed build wiring keeps the diagnostic macro off the ordinary benchmark targets.

6. Low Python version failure is explicit and early.

   Evidence: `run_benchmarks.py:192` through `run_benchmarks.py:194` rejects `sys.version_info < (3, 11)` with exit code `2` and a direct diagnostic. `python-version-rejection.json` records Python 3.10 invoking `run_benchmarks.py --help`, exit code `2`, stderr containing `Python 3.11+`, and validation verdict `PASS`. My shell's default `python` is also 3.10 and reaches the same diagnostic. No benchmark output directory is created before this guard because argument parsing and path resolution occur after it.

## Non-blocking notes

- `cmake --list-presets=all C08_Concurrency/exercises` succeeds. On this Windows host, Linux configure presets are hidden by their host condition, while Linux build/test presets still appear by name; they remain bound to Linux-only configure presets and were not executed here.
- The A2 ASan smoke artifacts prove the Windows MSVC RelWithDebInfo path for one exercise, not the whole C08 tree. That is acceptable for this infrastructure slice because the change is centralized in `cs_configure_target` and per-test timeout properties.
- The latest `StudySetup.cmake` also adds default-off C++29 probe plumbing. This review only checked that it does not disturb sanitizer or timeout behavior. The singular macro name `CS_HAS_STD_HAZARD_POINTER_BATCH` is used consistently by `NativeFeatures.cmake` and current consumers.

Stop condition: report written only to this review file; reviewed infrastructure files were not edited.
