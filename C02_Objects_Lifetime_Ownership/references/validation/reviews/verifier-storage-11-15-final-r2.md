# C02 independent verifier closeout — storage group 11-15 final fixes

## Verdict

APPROVE.

The prior storage blocker is closed. The original three ClangCL ASan Reference failures now build and run through `record_process.py`; each output directory contains a copied `clang_rt.asan_dynamic-x86_64.dll` whose SHA matches the active `clang-cl` resource directory. L12 quoted diagnostic handling rejects unrelated failures. L14 frontier now binds baseline fixtures, uses strict diagnostic matching, and rejects missing-target/bad-diagnostic controls.

This closeout supplements `verifier-storage-11-15-r1.md`, which had `REQUEST_CHANGES` only for the ClangCL ASan link failure.

## Evidence

Evidence directory: `Core_Study/references/validation/reviews/c02-storage-final-fix-verifier-evidence-r1/`.

Input binding: `input-binding.json`.

- `git_head`: `d81c136c342b62d66ceb8767ffc581e12ca2f5d2`
- `StudySetup.cmake`: `3D5F0EDB294012C2465AF14634BB3179703AF085B2C6B166674211E0F41FC26C`
- `record_process.py`: `A6CBE3E05A8A9845843254E17656E24C6F90E259F0AD93151A8AA09571246002`
- `L12_storage/CMakeLists.txt`: `9E87001A142415F70376BB067C3F0870A69F36BC139BDCB3267F5EFD7D744AA2`
- `L14_ub/CMakeLists.txt`: `9F1532AFBBC6F395F8AA38264C961346B2CB52CC481486FD76AA5D75FA2A6674`
- `L14_ub/checks/frontier_compile.cmake`: `47E07F26B41D2A76A4F562B6EE15F9F3315CB098A259C3AFFC398513CD796058`
- ClangCL path from CMake/resource check: `D:/VisualStudio2026/Installed/VC/Tools/Llvm/x64/bin/clang-cl.exe`
- Clang resource dir: `D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22`

Public helper / runner:

- `studysetup-asan-lines.txt` — current helper has the `CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"` Clang branch, avoids GNU link `-fsanitize=address` for that frontend, adds matching `clang_rt.asan_dynamic-*.lib`, `/WHOLEARCHIVE` dynamic runtime thunk, and `__asan_seh_interceptor`, and still copies matching runtime DLL with `NO_DEFAULT_PATH` lookup.
- `record-process-self-check-final-corrected.json` — PASS, current recorder self-check reports `PASS: 7 recorder contracts`.
- All recorded run/build commands in this closeout show `inherited_windows_error_mode=32771`, proving they went through the updated recorder path instead of naked launching known loader-failure binaries.

ClangCL ASan original three failures:

- `L12-clangcl-asan-reference-release-configure.json` — PASS.
- `L12-clangcl-asan-reference-release-build-target.json` — PASS; `L12_storage_reference.vcxproj -> ...\Release\L12_storage_reference.exe`.
- `L12-clangcl-asan-reference-release-run-target.json` — PASS; output `L12_storage_contract OK`.
- `P1-clangcl-asan-reference-release-configure.json` — PASS.
- `P1-clangcl-asan-reference-release-build-target.json` — PASS; `P1_object_buffer_reference.vcxproj -> ...\Release\P1_object_buffer_reference.exe`.
- `P1-clangcl-asan-reference-release-run-target.json` — PASS; output `P1_object_buffer_contract OK`.
- `L14-clangcl-asan-reference-release-configure.json` — PASS.
- `L14-clangcl-asan-reference-release-build-target.json` — PASS; `L14_ub_observation.vcxproj -> ...\Release\L14_ub_observation.exe`.
- `L14-clangcl-asan-reference-release-run-target.json` — PASS; output `L14_ub_observation OK`.

DLL matching:

- `clangcl-asan-dll-match.json` — PASS by inspection for L12/P1/L14. Each build dir uses compiler `D:/VisualStudio2026/Installed/VC/Tools/Llvm/x64/bin/clang-cl.exe`; source DLL is `D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows\clang_rt.asan_dynamic-x86_64.dll`; copied Release DLL exists; source and copied SHA both equal `0C77CDD870DBD719A3475CFB02507CC6B56D6DFA5D46FC960177CFE1A1298FF7`.

L12 full diagnostic handling:

- `L12-final-diagnostic-configure.json` — PASS.
- `L12-final-diagnostic-build-debug.json` — PASS.
- `L12-final-diagnostic-ctest-debug-verbose.json` — PASS; verbose command passes quoted `-DEXPECTED_TEXT=slot must report engaged after successful construct` and CTest reports `100% tests passed`.
- `L12-final-fake-slot-unrelated-negative.json` — PASS as negative proof; unrelated failing command exits 1 and current checker rejects it with `expected diagnostic not found`.

L14 frontier strict diagnostics and baseline fixture:

- `l14-final-source-lines.txt` — source inspection shows strict diagnostic extraction from warning/error lines, baseline `FIXTURES_SETUP L14_frontier_baseline_ready`, other frontier probes `FIXTURES_REQUIRED L14_frontier_baseline_ready`, `RESOURCE_LOCK`, and specific P2287/P2748/P2953 patterns.
- `L14-final-frontier-configure.json` — PASS.
- `L14-final-frontier-build-all-debug.json` — PASS; unsupported frontier probes are not pulled into normal build-all.
- `L14-final-frontier-ctest-NV-corrected.json` — PASS; lists 6 tests.
- `L14-final-frontier-generated-fixture-lines.txt` — generated `CTestTestfile.cmake` has `FIXTURES_SETUP "L14_frontier_baseline_ready"` on baseline and `FIXTURES_REQUIRED "L14_frontier_baseline_ready"` on P2287/P2748/P2953/provenance.
- `L14-final-frontier-ctest-debug-verbose.json` — PASS; baseline compiles, P2287 SKIPs only after expected `p2287_base_member_designator.cpp` diagnostic, P2748 SKIPs after expected diagnostic, P2953 is classified SKIP, provenance compiles with `no runtime support claim`, and CTest reports `100% tests passed`.
- `L14-final-frontier-missing-target-negative.json` — PASS as negative proof; missing target under `capability_reject` exits 1 with `failure did not mention expected source`.
- `L14-final-frontier-bad-diagnostic-negative.json` — PASS as negative proof; real P2287 source with intentionally wrong diagnostic exits 1 with `failure did not match expected diagnostic`.

Previously approved evidence still stands for unaffected storage checks:

- `verifier-storage-11-15-r1.md` recorded fresh MSVC Debug/Release leaf PASS for L11/L12/L13/L14/P1, L12/P1 Student/ref-off PASS, P1 good/bad/failure-injection PASS, MSVC ASan safe PASS, and L14 ASAN+UNSAFE UAF diagnostic PASS.

## Gaps

- No remaining blocker in the requested final slice. I did not expand into additional undeclared platform/toolchain matrices.

## Risks

- This remains bounded course-contract verification. It does not claim a sandbox against arbitrary malicious C++ or validate unrequested toolchains beyond the declared MSVC/ClangCL paths in this slice.
