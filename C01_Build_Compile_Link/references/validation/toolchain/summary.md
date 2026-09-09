# F-I toolchain validation summary

## Scope

This directory records validation for chapters 06-09 and exercises F1/F2/G1/G2/H1/I1. Commands were run from `F:\CPPTrain\LearnCPP` in a normal local process using VS child environment only. No global configuration, installs outside the workspace, commits, pushes, or third-party downloads were performed.

## Current G1/G2/H1 closure evidence

- `580-verify-g1-f2-checker-strength.*`: current G1 Reference/Student share the same exhaustive contract checker over all 100 two-digit ASCII inputs plus invalid classes. A copied good parser passes Reference and Student; a copied constant-42 mutant builds but fails Student with `parser must accept every two-digit ASCII input from 00 to 99`. Current F2 adds `F2_dependencies_student_delegation`, which compiles real `src/student/student.cpp` against a spy provider and does not link the real provider. A copied good student passes real provider and spy checks; a copied `input * 2 + 2` formula mutant passes the real provider value test but fails the spy delegation test with `student must return the provider result without recomputing or adjusting it`. Exit 0.
- `578-verify-g1-safe-starter-current.*`: current G1 Student starter is a safe TODO placeholder returning `-1`, outside the valid `0..99` result domain. Fresh Debug and Release builds compile `G1_diagnostics_student`; direct runs exit 1 with the expected check failure message, without timeout or unsafe parser execution. Reference CTest passes in both configurations. Exit 0.
- `577-verify-g1-fuzz-after-safe-starter.*`: current G1 libFuzzer harness links `fuzz_parse.cpp` with the real `reference/parser.cpp` and separately links `fuzz_bad/always_42_parser.cpp` after the safe Student change. Fixed seed corpus contains only `42`, `00`, `4x`, and `7`. Real parser passes `-runs=16 -seed=20260908 -max_total_time=5`; bad parser is accepted as rejected only because stderr contains `G1_ORACLE_MISMATCH:` with nonzero exit, no timeout, no runner error, and no cleanup error. Exit 0.
- `574-verify-g1-fuzz-output-exists-rejected.*`: expected-failure wrapper reruns `verify_fuzz.py` against an existing output directory and treats argparse `output already exists` rejection as PASS. Exit 0.
- `569-verify-g1-cmake-current.*`: fresh G1 CMake configure/build/CTest with root-updated `check.hpp`. Reference parser valid parse and `std::invalid_argument` rejection pass. Exit 0.
- `570-verify-g2-shared-runner-selftest.*`: `measure_build.py --self-test` using public `Engineering_Study/exercises/tools/process_runner.py`; existing output rejection, nonzero command classification, and timeout classification all pass. Exit 0.
- `571-verify-g2-shared-runner-subset.*`: scoped G2 driver smoke requests only `variant=baseline` and `scenario=noop`. `report.json` status PASS, summary length is exactly 1, and report records `process_runner_sha256=77f7aa8a86ff5fa4fdb933c8bb9929d637748a1aa00fb7afb3dac941e47d936b`. Exit 0.
- `573-verify-h1-boundaries.*`: fresh H1 module boundary validation builds and runs `H1_modules_reference`, `H1_visibility_boundary`, `H1_global_private_fragment`, and package consumer. It then builds `H1_hidden_type_negative` expecting compile failure and confirms MSVC `C2065` for direct `hidden_state` naming. Exit 0.
- `572-verify-h1-boundaries.*`: retained failed attempt. H1 positive tests passed and the negative target failed with `hidden_state` undeclared, but the script matched the redirected log with a brittle check and exited 14. Superseded by `573`.
- `576-verify-g1-safe-starter.*`: retained prior G1 safe-starter pass whose batch script passed but wrote an empty student exit file because delayed expansion was missing; superseded by `578`.
- `575-verify-g1-fuzz-strict-marker-current.*`: retained prior G1 strict-marker pass before the safe Student starter change; superseded by `577`.
- `568-verify-g1-fuzz-strict-marker.*`: retained prior G1 strict-marker pass before the final `error == ""` predicate was added; superseded by `575` and `577`.
- `567-verify-clang-tools-current-fresh.*`: current Clang tools check. ASan safe parser passes, ASan unsafe fault reports `heap-buffer-overflow`, current libFuzzer command links the real parser and runs copied seed corpus, G2 ftime trace emits five TU JSON files. Exit 0. LNK4217 warnings from `clang_rt.ubsan_standalone` during fuzzer link are recorded as expected for this toolchain combination.
- `558-verify-g2-subset-smoke-fresh.*` and `559-verify-g2-variants-fresh.*`: retained prior G2 closure before the shared public process runner extraction. Superseded for driver evidence by `570`/`571`; `559` still records a fresh CMake build where baseline/PCH/LTO all return 42.

## Earlier evidence retained

- `579-verify-f2-fetchcontent-local-source.*`: current F2 `FetchContent_Declare` uses explicit local `SOURCE_DIR` from `F2_PROVIDER_FIXTURE_SOURCE_DIR`, with no placeholder URL or all-zero hash. Fresh default offline configure/build/CTest passes and build output shows `provider_fixture/src/provider.cpp` compiled under `_deps`. A separate configure with missing `F2_PROVIDER_FIXTURE_SOURCE_DIR` fails at configure with the expected fatal message. Exit 0.
- `500-verify-fghi.*`: F1/F2/G1/G2/H1/I1 leaf CMake configure/build/test chain from the previous pass. It is retained for history; current G1/G2 closure should use the newer 564/565/558/559/567 records above.
- `510-verify-i1.*`: focused I1 CMake import std rebuild. Exit 0.
- `520-verify-clang-tools.*` / `566-verify-clang-tools-current.*`: retained historical Clang tool records. `567` is the current fresh record after moving fuzzer corpus mutation out of the source tree.
- `525-verify-static-analysis.*`: Clang static analyzer reference parser plist plus deliberate `core.NullDereference` observation sample. Exit 0.
- `530-verify-i1-direct-cl.*`: direct MSVC `std.ixx` compile, importer compile with `/reference std=std.ifc`, link and run. Exit 0, stdout ends with `10`.
- `540-verify-f2-package.*`: provider fixture install/export, `find_package(F2Provider 1.0 CONFIG REQUIRED)` consumer build/test, and expected `find_package(F2Provider 2.0 CONFIG REQUIRED)` configure failure against installed version 1.0.0. Exit 0.
- `545-verify-f2-fetchcontent.*`: historical offline `FetchContent` build before removing placeholder URL/hash. Superseded by `579` for current F2 FetchContent evidence. Exit 0.
- `550-verify-g2-driver-selftest.*`: driver self-test rejects existing output, classifies nonzero commands as FAIL, and classifies timeout as FAIL with `timeout=true`. Exit 0.
- `555-verify-g2-driver-preflight.*`: G2 driver preflight using 1 warmup + 1 sample across all 3 variants and all 4 scenarios. Current valid output is `g2-preflight-author-r2`; earlier `g2-preflight-author` is superseded because its old action parser overcounted sources from link command text.
- `556-verify-g2-variants.*` / `556-r3-verify-g2-variants.*`: retained G2 variant checks. `559` is the current fresh builddir record.
- `sources-sha256.txt`: SHA-256 fingerprints for F-I chapters, exercise source files, `exercises/tools/process_runner.py`, and fresh validation command scripts.

## Toolchain

- CMake: `D:\cmake\install\bin\cmake.exe` 4.2.3
- Ninja: `D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe` 1.13.2
- MSVC environment: `D:\VisualStudio2026\Installed\Common7\Tools\VsDevCmd.bat -no_logo -arch=x64`
- MSVC toolset observed in commands: 14.51.36231 / compiler version 19.51
- Clang: `D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin\clang++.exe`
- Clang runtime PATH prefix for ASan: `D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows`

## G2 driver boundary

`G2_build_cost/scripts/measure_build.py` is the formal driver. It supports `--output`, `--work-root`, `--samples`, `--warmups`, `--seed`, `--parallel`, `--timeout`, `--variant`, and `--scenario`. `--output` and `--work-root` are required to be new directories when used. Per-round source/build copies live under `--work-root`; raw command stdout/stderr/json and `report.json` live under `--output`.

The author preflight intentionally used only 1 formal sample to prove the complete matrix and data shape. Root should run the same driver with `--samples 5 --warmups 1` after other authors stop building before treating medians/ranges as formal performance data.

CMake `compiler working skipped` log text, when seen after ABI success in the same configure, means CMake reused earlier compiler evidence. It is not treated as a capability skip or a forced `CMAKE_CXX_COMPILER_WORKS` override.
