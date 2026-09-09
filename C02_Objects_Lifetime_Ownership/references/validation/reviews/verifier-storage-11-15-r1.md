# C02 independent verifier report — storage group 11-15

## Verdict

REQUEST_CHANGES.

Non-ASan storage-group checks pass. L14 frontier r2 checks pass. MSVC ASan safe/unsafe checks pass after the runtime deployment fix. The remaining blocker is ClangCL ASan: Reference targets do not link because the Visual Studio ClangCL generator invokes `lld-link`, which ignores `-fsanitize=address`; the resulting link fails on unresolved `__asan_*` symbols. This fails the requested Clang ASan safe Reference criterion.

## Evidence

Evidence directory: `Core_Study/references/validation/reviews/c02-storage-verifier-evidence-r1/`.

Current input binding after ASan/runtime fixes: `final-current-input-binding-post-asan-fix.json`.

- `cwd`: `F:\CPPTrain\LearnCPP`
- `git_head`: `d81c136c342b62d66ceb8767ffc581e12ca2f5d2`
- CMake: `cmake version 4.2.3`
- Python: `Python 3.10.11`
- ClangCL: `clang version 22.1.3`

Fresh MSVC Debug/Release leaf matrix:

- L11: `L11-configure-default.json`, `L11-build-Debug.json`, `L11-ctest-Debug.json`, `L11-build-Release.json`, `L11-ctest-Release.json` — PASS.
- L12: original matrix passed; after L12 CMake changed, current rerun `L12-current-configure-default.json`, `L12-current-build-Debug.json`, `L12-current-ctest-Debug-verbose.json`, `L12-current-build-Release.json`, `L12-current-ctest-Release-verbose.json` — PASS.
- L13: `L13-configure-default.json`, `L13-build-Debug.json`, `L13-ctest-Debug.json`, `L13-build-Release.json`, `L13-ctest-Release.json` — PASS.
- L14 default: `L14-configure-default.json`, `L14-build-Debug.json`, `L14-ctest-Debug.json`, `L14-build-Release.json`, `L14-ctest-Release.json` — PASS.
- P1: `P1-configure-default.json`, `P1-build-Debug.json`, `P1-ctest-Debug.json`, `P1-build-Release.json`, `P1-ctest-Release.json` — PASS.

L12 current targeted checks:

- `record-process-self-check-storage-r1.json` — PASS; updated recorder self-check reports `PASS: 6 recorder contracts`.
- `L12-current-reference-run-Debug.json`, `L12-current-reference-run-Release.json` — PASS, `L12_storage_contract OK`.
- `L12-current-good-run-Debug.json`, `L12-current-good-run-Release.json` — PASS, `L12_storage_contract OK`.
- `L12-current-bad-noop-run-Debug.json`, `L12-current-bad-noop-run-Release.json` — PASS as negative checks, exit 1 with `slot must report engaged after successful construct`.
- `L12-current-start-lifetime-run-Debug.json`, `L12-current-start-lifetime-run-Release.json` — PASS; capability path actually ran with `L12_start_lifetime_as OK macro=202207`, not SKIP.
- `L12-current-configure-student-ref-off.json`, `L12-current-build-student-ref-off-Debug.json`, `L12-current-build-student-ref-off-Release.json` — PASS.
- `L12-current-student-placeholder-ref-off-Debug.json`, `L12-current-student-placeholder-ref-off-Release.json` — PASS as negative checks, placeholder rejected by `slot must report engaged after successful construct`.
- `L12-current-reference-target-absent-ref-off.json` — PASS as negative check; reference target absent in ref-off tree.
- `L12-current-fake-slot-unrelated-negative.json` — PASS as negative proof; fake command exits nonzero with unrelated text and current `expect_failure.cmake` rejects it via `expected diagnostic not found`.

P1 targeted checks:

- `P1-reference-run-Debug-corrected.json`, `P1-reference-run-Release-corrected.json` — PASS, `P1_object_buffer_contract OK`.
- `P1-good-run-Debug.json`, `P1-good-run-Release.json` — PASS, `P1_object_buffer_contract OK`.
- `P1-bad-noop-run-Debug.json`, `P1-bad-noop-run-Release.json` — PASS as negative checks, exit 1 with `reserve grows capacity`.
- `P1-bad-early-commit-run-Debug.json`, `P1-bad-early-commit-run-Release.json` — PASS as negative checks, exit 1 with `new tail failure keeps capacity`.
- `P1-reject-throwing-move-only-build-Debug-direct.json`, `P1-reject-throwing-move-only-build-Release-direct.json` — PASS as compile-negative checks, exit 1 with `copy constructible or nothrow move constructible`.
- `P1-configure-student-ref-off.json`, `P1-build-student-ref-off-Debug.json`, `P1-build-student-ref-off-Release.json` — PASS.
- `P1-student-placeholder-ref-off-Debug.json`, `P1-student-placeholder-ref-off-Release.json` — PASS as negative checks, placeholder rejected by `reserve grows capacity`.
- `P1-reference-target-absent-ref-off.json` — PASS as negative check.
- `p1-contract-source-lines.txt` — checker/source evidence covers new-tail failure before moving old elements, reserve prefix rollback, old-copy cleanup, self-view append, move-only transfer, borrowing follows move target, target old borrowing invalidation, self move, over-aligned storage, and no remaining live objects.

L14 frontier checks:

- Old snapshot blocker preserved: `L14-frontier-build-current-expected-fail.json` shows frontier build failed on `p2287_base_member_designator.cpp` before `EXCLUDE_FROM_ALL`.
- Current source inspection: `l14-current-after-change-lines.txt` shows `OBJECT EXCLUDE_FROM_ALL`, `SOURCE_MARKER`, `EXPECTED_FAILURE`, `ACCEPTED_DIAGNOSTIC`, `OUT_DIR`, `RESOURCE_LOCK L14_frontier_build_tree`, and internal command timeout.
- `L14-frontier-r2-current-configure.json` — PASS.
- `L14-frontier-r2-current-build-all-debug.json` — PASS; build-all no longer compiles unsupported frontier probes.
- `L14-frontier-r2-current-ctest-debug-verbose.json`, `l14-frontier-current-ctest-extract.txt` — PASS; baseline compiles, P2287 skips only with expected source/diagnostic, P2748/P2953 skip after accepted diagnostics, provenance is compile-only with no runtime support claim.
- `L14-frontier-r2-current-missing-target-negative.json` — PASS as negative proof; missing target under `capability_reject` exits 1 because output lacks expected source.
- `L14-frontier-r2-current-baseline-missing-target-negative.json` — PASS as negative proof; missing baseline target exits 1 as baseline/toolchain failure.

MSVC ASan after runtime deployment fix:

- `L12-msvc-asan-safe-current-configure.json`, `L12-msvc-asan-safe-current-build-debug.json`, `L12-msvc-asan-safe-current-ctest-debug.json` — PASS, safe ASan CTest passes.
- `P1-msvc-asan-safe-current-configure.json`, `P1-msvc-asan-safe-current-build-debug.json`, `P1-msvc-asan-safe-current-ctest-debug.json` — PASS, safe ASan CTest passes.
- `L14-msvc-asan-safe-current-configure.json`, `L14-msvc-asan-safe-current-build-debug.json`, `L14-msvc-asan-safe-current-ctest-debug.json` — PASS, safe ASan CTest passes.
- `L14-msvc-asan-safe-current-no-uaf-test.json` — PASS, default safe ASan has `Total Tests: 1`; unsafe UAF is not registered by default.
- `L14-msvc-asan-unsafe-current-configure.json`, `L14-msvc-asan-unsafe-current-build-debug.json`, `L14-msvc-asan-unsafe-current-ctest-uaf-debug.json` — PASS; ASAN+UNSAFE runs the isolated UAF test and matches `heap-use-after-free` plus `asan_uaf_probe.cpp`.

ClangCL ASan blocker:

- `L12-clangcl-asan-reference-release-current-build-target.json` — FAIL. Command: `cmake --build build/c02-storage-verifier-r1/L12-clangcl-asan-reference-release-current --config Release --clean-first --target L12_storage_reference`. Output includes `lld-link : warning : ignoring unknown argument '-fsanitize=address'` and unresolved `__asan_shadow_memory_dynamic_address`, `__asan_init`, `__asan_version_mismatch_check_v8`, etc.
- `P1-clangcl-asan-reference-release-current-build-target.json` — FAIL with the same `lld-link` ignored sanitizer flag and unresolved `__asan_*` symbols for `P1_object_buffer_reference`.
- `L14-clangcl-asan-reference-release-current-build-target.json` — FAIL with the same `lld-link` ignored sanitizer flag and unresolved `__asan_*` symbols for `L14_ub_observation`.
- `clangcl-asan-link-diagnostics.txt` — confirms `CMAKE_GENERATOR_TOOLSET=ClangCL` and `CMAKE_LINKER=.../lld-link.exe`; the current `target_link_options(... -fsanitize=address)` is reaching `lld-link`, where it is ignored.
- `asan-post-fix-extract.txt` — compact extract of the above ClangCL failures and MSVC ASan passes.

Prior ASan loader diagnostics returned to root:

- `L14-asan-safe-default-runtime-diagnostics.md` records the earlier MSVC ASan runtime mismatch before root fix: the problematic exe linked MSVC ASan import libs from `D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\LIB\X64\`; raw CTest exited `0xc0000135`, LLVM-PATH retry exited `0xc0000139`. No DLL was copied by me.

## Gaps

- I did not run a successful Clang ASan safe Reference because current ClangCL builds fail at link before any executable is produced.

## Risks

- Minimal repair: handle Visual Studio `-T ClangCL` separately. Do not pass GNU-style `-fsanitize=address` to `lld-link`; either add the matching Clang ASan import libraries from the compiler resource dir to the link inputs, or arrange for the clang-cl driver to perform the link. Keep the existing matching-DLL copy rule and no-substitution check.
- The rest of the checked storage slice is evidence-backed, but the batch should not be approved until Clang ASan Reference builds and runs cleanly or the acceptance criterion is explicitly changed to MSVC ASan only.
