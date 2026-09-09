# Final technical review pre-gate

Current verdict: APPROVE for the C01 technical implementation and final data gate. Historical pre-gate verdict below remains for traceability.

This is a non-author review of the latest root minimal technical changes. It does not re-review the six F-I README teaching drafts, which root stated are under a separate documentation review. It also does not treat the in-progress/preliminary final matrix as formal completion data.

Snapshot observed: 2026-09-08, `HEAD=c65edb3c2b5c76f386e53fa0afce87c54eb0c363` with `Engineering_Study/` still untracked in git status.

## Scope reviewed

- `Engineering_Study/exercises/F1_cmake_targets/checks/reference_check.cpp`
- `Engineering_Study/exercises/F1_cmake_targets/checks/student_check.cpp`
- `Engineering_Study/exercises/F2_dependencies/CMakeLists.txt`
- `Engineering_Study/exercises/F2_dependencies/checks/reference_check.cpp`
- `Engineering_Study/exercises/F2_dependencies/checks/student_check.cpp`
- `Engineering_Study/exercises/F2_dependencies/provider_fixture/src/provider.cpp`
- `Engineering_Study/exercises/G2_build_cost/scripts/measure_build.py`
- `Engineering_Study/references/validation/run_integration.py`
- relevant validation records under `Engineering_Study/references/validation/toolchain/` and `Engineering_Study/references/validation/integration/final-matrix-r1/`

## Issues

### HIGH 1 - Current final integration matrix fails in `modules-msvc-ninja`

Files:

- `Engineering_Study/references/validation/integration/final-matrix-r1/report.json:1`
- `Engineering_Study/references/validation/integration/final-matrix-r1/modules-msvc-ninja-ctest.json:1`
- `Engineering_Study/exercises/C2_archive/cmake/check_symbols.cmake:19`
- `Engineering_Study/exercises/C2_archive/cmake/check_symbols.cmake:29`

Trigger:

`run_integration.py` wrote `Engineering_Study/references/validation/integration/final-matrix-r1/report.json` with `status=FAIL`. The last recorded step is `modules-msvc-ninja-ctest`, exit 8, no timeout, no cleanup error.

Evidence:

`modules-msvc-ninja-ctest.stdout.txt` shows 39 tests run, with 2 initial failures. After a manual target rebuild/regeneration, `C1_odr_observation_symbols` passed, but `C2_archive_symbols` still failed:

```text
expected one used_member object for config Release, found 0 matching config out of 2:
.../C2_archive_library.dir/src/reference/used_member.cpp.obj;
.../C2_archive_negative_direct_objects.dir/used_member.cpp.obj
```

Root cause in current script: `check_symbols.cmake:19-26` collects every `used_member.cpp.obj` under the binary tree, including the negative target object. `_select_config()` at lines 29-48 can disambiguate multi-config paths containing `/Release/`, but single-config Ninja object paths do not contain `/Release/`. With two candidates and zero config-path matches, the observation test fails even though the reference archive target itself is built.

Impact:

The final matrix cannot be accepted as a green implementation/experiment gate. This is outside the narrow F-I code changes, but `run_integration.py` includes this preset in the final gate, so the current final evidence is failing and must not be summarized as pass.

Minimum fix:

Constrain `C2_archive_symbols` to the intended target object path, for example only objects under `C2_archive/CMakeFiles/C2_archive_library.dir/`, or pass the expected target object directory/file from CMake instead of globbing all `used_member` objects. Then rerun the final matrix from fresh output/build directories.

### HIGH 2 - `sources-sha256.txt` does not bind the current reviewed code snapshot

File: `Engineering_Study/references/validation/toolchain/sources-sha256.txt:7`

Trigger:

I rechecked the manifest against current files after root's latest technical edits.

Evidence:

The manifest no longer matches several current files. Mismatches include code, not only README drafts:

- `sources-sha256.txt:7`: `Engineering_Study/exercises/F1_cmake_targets/checks/reference_check.cpp`
- `sources-sha256.txt:8`: `Engineering_Study/exercises/F1_cmake_targets/checks/student_check.cpp`
- `sources-sha256.txt:15`: `Engineering_Study/exercises/F2_dependencies/checks/reference_check.cpp`
- `sources-sha256.txt:16`: `Engineering_Study/exercises/F2_dependencies/checks/student_check.cpp`
- `sources-sha256.txt:54`: `Engineering_Study/exercises/G2_build_cost/scripts/measure_build.py`

Current hashes observed for key changed files:

```text
A3BCE3A7EA0E54901D26F59B78D4EBFBCF5CFBC8B9AAA7BEA9FDAF249FD91313  Engineering_Study/exercises/F1_cmake_targets/checks/reference_check.cpp
7D0FD53C0AD2E3BB4F34DA8EA4B9915EDAEF84486B92EFA920796266A38DFC8D  Engineering_Study/exercises/F1_cmake_targets/checks/student_check.cpp
0E96AEB92EBED161EAAB1BAD6B5510A5B4F4510FAF64F42C1A85E2B731D71117  Engineering_Study/exercises/F2_dependencies/checks/reference_check.cpp
364E477053AEC7F9926E68E8503C872F7057860FCC1706420842182ACD8B3AA0  Engineering_Study/exercises/F2_dependencies/checks/student_check.cpp
E93B3068A0FE1010C2E9613910948DF4A4BE44B1E7D51C47FCFCB9D495A93BCC  Engineering_Study/exercises/G2_build_cost/scripts/measure_build.py
1852204E4DDF31B58F74518E09FDDA872F3061E6BF665AFDC8439034C3B2CF2A  Engineering_Study/references/validation/run_integration.py
```

Impact:

The review cannot use the existing manifest as the final source identity for the current technical snapshot. Since the changed files include checks and the G2 driver, this is a final evidence binding blocker.

Minimum fix:

Regenerate the fingerprint manifest after the final code/helper/script state is frozen. Keep README draft fingerprints separate if README review is intentionally still in flight, or clearly label which manifest binds the technical code snapshot and which one binds documentation.

## Passed targeted checks

### F1 constant-42 bad variant rejected

Temporary source only under `Engineering_Study/exercises/build/final-technical-review/f1-constant-src`; course source was not modified.

Change in temp variant:

```cpp
int student_add_scaled(int, int) { return 42; }
```

Commands: fresh CMake/Ninja Release configure/build for F1 with `ENGINEERING_STUDY_TEST_STUDENTS=ON`, then separate Reference and Student CTest.

Result:

- Reference CTest exit 0.
- Student CTest exit 8, no timeout, with `check failed: student result must not be a constant 42`.

This proves the new F1 checks reject the representative constant implementation.

### F2 constant-42 bad variant rejected

Temporary source only under `Engineering_Study/exercises/build/final-technical-review/f2-constant-src`; course source was not modified.

Change in temp variant:

```cpp
int student_use_dependency(int) { return 42; }
```

Commands: fresh CMake/Ninja Release configure/build for F2 default `fetchcontent` with `ENGINEERING_STUDY_TEST_STUDENTS=ON`, then separate Reference and Student CTest.

Result:

- Reference CTest exit 0.
- Student CTest exit 8, no timeout, with `check failed: student result must not be a constant 42`.

This proves the new F2 checks reject the representative constant implementation.

### F2 FetchContent local source shape is correct

`Engineering_Study/exercises/F2_dependencies/CMakeLists.txt:22-24` now uses only:

```cmake
FetchContent_Declare(f2_provider_fixture
    SOURCE_DIR "${F2_PROVIDER_FIXTURE_SOURCE_DIR}"
)
```

There is no placeholder URL or fake `URL_HASH`. Lines 17-20 explicitly reject a missing local fixture source tree. Author record `579-verify-f2-fetchcontent-local-source.*` documents valid local-source build and missing-source configure failure. I did not rerun 579 because the constant-variant F2 build above independently exercised the default local FetchContent path and compiled `provider_fixture/src/provider.cpp` under `_deps`.

### G2 command defaults and reusable runner are acceptable

`Engineering_Study/exercises/G2_build_cost/scripts/measure_build.py:248-249` now defaults to `cmake` from PATH and `shutil.which("ninja") or "ninja"`, while still allowing explicit `--cmake` and `--ninja`. This removes hardcoded local tool paths from the shipped script without changing the measurement algorithm.

`run_integration.py` and the G1/G2 helper scripts passed Python syntax checks:

```text
python -m py_compile run_integration.py measure_build.py verify_fuzz.py process_runner.py
py_compile_exit=0
```

### `run_integration.py` overwrite protection and failure propagation are correct

`Engineering_Study/references/validation/run_integration.py:28-31` rejects both an existing output directory and an existing matching build directory before running CMake. I verified both cases:

```text
existing output: exit 2, "output and its matching build directory must both be new"
existing build:  exit 2, same diagnostic
```

The preliminary final matrix failure also shows failure propagation works: `modules-msvc-ninja-ctest` returned status FAIL and `report.json` was marked FAIL instead of continuing to a false PASS.

## Static/quality checks

Pattern scan over the changed technical area found no `assert(`, empty catch block, bare `except:`, `apiKey=`, or `console.log`.

No LSP diagnostics tool is available in this reviewer surface. I used C++ compile/CTest evidence, Python `py_compile`, static grep, and raw validation log inspection instead.

## Boundaries

- I did not run the full final matrix myself; root's preliminary `final-matrix-r1` was read only after it appeared.
- I did not run G2 formal 72-round data collection. No performance conclusion is approved here.
- I did not approve the six changing F-I README teaching drafts.
- I did not expand scope to unrelated dirty Coroutine/Concurrency/P2 files.

## Recommendation

REQUEST_CHANGES for the final technical gate. The F1/F2 root changes and reusable integration script mechanics are mostly sound, but the final matrix currently fails in `C2_archive_symbols`, and the source fingerprint manifest does not match current changed code. Fix those two blockers, rerun fresh final integration output/build directories, regenerate current fingerprints, then request the final data/technical re-review.

---

# Final technical and data gate sign-off

Verdict: APPROVE for the C01 technical implementation and final data gate.

This final section supersedes the pre-gate `REQUEST_CHANGES` verdict above. The historical findings are intentionally kept in this file; the evidence below closes them against the final frozen delivery snapshot.

Final snapshot observed: 2026-09-08, `HEAD=c65edb3c2b5c76f386e53fa0afce87c54eb0c363`, with `Engineering_Study/` still untracked in git status as expected for this delivery workspace.

## Final delivery fingerprints

- `Engineering_Study/references/validation/snapshots/delivery/source.sha256`: 251 lines, file SHA256 `6d310aba7509f4d909081d9bfeb66a255e5d485d3b83c563ee694e9105242948`.
- `Engineering_Study/references/validation/snapshots/delivery/evidence.sha256`: 3050 lines, file SHA256 `4191ed568e5ec3bc5aa80d6a4284aac8b7483e09fdae2e22778d21b70940cb9e`.
- I independently walked every listed entry in both manifests and found no missing file and no hash mismatch.
- Spot-checked final manifest coverage includes the previously volatile files: `C2_archive/cmake/check_symbols.cmake`, F1/F2 checkers, F2 spy provider/delegation checks, `G2_build_cost/scripts/measure_build.py`, `quality-report.md`, `measurements/README.md`, and `measurements/g2-formal-r1/report.json`.

## Closed HIGH 1: final matrix no longer fails at C2 archive symbols

Final evidence:

- `Engineering_Study/references/validation/integration/final-matrix-r3/report.json`: `status=PASS`, 12/12 configure-build-ctest steps PASS, exit 0, no timeout.
- JUnit totals: `verify-core.xml` 34 tests / 0 failures / 0 disabled / 0 skipped; `verify-debug.xml` 34 / 0 / 0 / 0; `modules-msvc-ninja.xml` 39 / 0 / 0 / 0; `import-std-msvc-ninja.xml` 40 / 0 / 0 / 0. Total 147 tests PASS.
- `Engineering_Study/references/validation/integration/c2-repeat-final.xml`: 2 tests / 0 failures, including `C2_archive_symbols`.

Code evidence:

- `Engineering_Study/exercises/C2_archive/cmake/check_symbols.cmake` now filters candidate objects to `/C2_archive_library.dir/` and `used_member` object names before config selection. This removes the prior contamination from negative targets under the same binary tree.
- Current file SHA256: `d36e7578d10b08fef62539fc0056d2649eb85ea679846180abde905ffb8264d2`.

## Closed HIGH 2: final manifest now binds the reviewed delivery snapshot

The old `toolchain/sources-sha256.txt` is historical. The final delivery manifest is under `references/validation/snapshots/delivery/` and matches the files currently on disk. The final source manifest includes code files changed after the previous pre-gate review, including:

- `Engineering_Study/exercises/F1_cmake_targets/checks/reference_check.cpp`: `a3bce3a7ea0e54901d26f59b78d4ebfbcf5cfbc8b9aaa7bea9fdaf249fd91313`.
- `Engineering_Study/exercises/F1_cmake_targets/checks/student_check.cpp`: `7d0fd53c0ad2e3bb4f34da8ea4b9915edaef84486b92efa920796266a38dfc8d`.
- `Engineering_Study/exercises/F2_dependencies/checks/delegation_check.cpp`: `3c1d8520318efea0b6dcd91e24d7e0f5613656cbd12e9fa5148a0f527628553d`.
- `Engineering_Study/exercises/F2_dependencies/checks/reference_check.cpp`: `0e96aeb92ebed161eaab1bad6b5510a5b4f4510faf64f42c1a85e2b731d71117`.
- `Engineering_Study/exercises/F2_dependencies/checks/student_check.cpp`: `364e477053aec7f9926e68e8503c872f7057860fcc1706420842182acd8b3aa0`.
- `Engineering_Study/exercises/G2_build_cost/scripts/measure_build.py`: `e13e73623f4752aec9ef9980f3f3a3a79d5a2fb47d8dd5932e7b3032c626c093`.
- `Engineering_Study/references/quality-report.md`: `9f721d9ca99281051657f429f2c5f77dcd6e187b3de6cbdf79e3c13575e9505f`.

## G2 formal measurement audit

Final G2 report:

- `Engineering_Study/references/validation/measurements/g2-formal-r1/report.json`: SHA256 `c828360c1c4d2910786cea31e9edf8b67f70bfb5ab69ff6c23b470bd2e4499a5`.
- `status=PASS`, 72/72 rounds PASS: 12 warmup rounds and 60 formal sample rounds.
- Raw process metadata count: 270 JSON files; all recorded `status=PASS`, `exit_code=0`, `timeout=false`, empty `error`, empty `cleanup_error`.
- Current fixture tree SHA256 equals report fixture SHA256: `b27230ad278ac603d5b884cb25df35fd850d54db341a7b10c5cd5a8b9fe08fc8`.
- Current shared runner SHA256 equals report runner SHA256: `77f7aa8a86ff5fa4fdb933c8bb9929d637748a1aa00fb7afb3dac941e47d936b`.
- Each round source tree hash and executable hash matched the files in its retained work directory.

Independent recalculation of all formal sample statistics matched `report.json` and `measurements/README.md`:

| variant | scenario | n | median_ms | min_ms | max_ms | range_ms |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| baseline | clean | 5 | 5615.880500 | 5454.177700 | 5733.558100 | 279.380400 |
| baseline | noop | 5 | 116.413200 | 100.166500 | 120.506500 | 20.340000 |
| baseline | implementation | 5 | 970.767500 | 846.443800 | 1037.213700 | 190.769900 |
| baseline | public-header | 5 | 3567.949900 | 3504.324200 | 3720.925900 | 216.601700 |
| pch | clean | 5 | 4131.356700 | 3852.660300 | 4352.238800 | 499.578500 |
| pch | noop | 5 | 99.416800 | 96.350600 | 106.695400 | 10.344800 |
| pch | implementation | 5 | 486.008500 | 482.809700 | 844.853300 | 362.043600 |
| pch | public-header | 5 | 2204.199500 | 2091.265500 | 2297.299200 | 206.033700 |
| lto | clean | 5 | 5559.462600 | 5440.455800 | 5981.886900 | 541.431100 |
| lto | noop | 5 | 123.209100 | 112.023700 | 154.693900 | 42.670200 |
| lto | implementation | 5 | 1035.652400 | 971.172300 | 1073.969300 | 102.797000 |
| lto | public-header | 5 | 3657.245900 | 3596.858100 | 3991.270700 | 394.412600 |

Compile-action audit from timed build metadata:

- All `noop` timed builds: 0 compile lines, no link, Ninja `no work to do`.
- All `implementation` timed builds: only `common.cpp` compiled, link seen.
- `baseline` and `lto` `clean` / `public-header`: five source TUs compiled: `alpha.cpp`, `beta.cpp`, `common.cpp`, `gamma.cpp`, `main.cpp`.
- `pch` `clean` / `public-header`: six compile lines; the five source TUs are seen plus one PCH-generation compile action.

The measurement README states the right policy limits: process time rather than pure compiler CPU time, no cache clearing/affinity/power-policy manipulation, 5 samples as observation rather than statistical proof, and no claim that PCH/LTO must monotonically improve all scenarios.

## G1 final contract audit

Final G1 code and evidence close the previous safety/checker issues:

- `Engineering_Study/exercises/G1_diagnostics/src/student/parser.cpp` is a safe TODO starter: it does not index `text[0]` or `text[1]`; it returns `-1` and fails normally.
- `Engineering_Study/exercises/G1_diagnostics/checks/parser_contract.hpp` tests all `00..99`, catches unexpected exceptions on valid input and reports a controlled `check failed`, and requires invalid inputs to throw `std::invalid_argument` rather than another exception type.
- `Engineering_Study/references/validation/integration/g1-final-Debug.xml`: 1 Reference test / 0 failures.
- `Engineering_Study/references/validation/integration/g1-final-Release.xml`: 1 Reference test / 0 failures.
- `Engineering_Study/references/validation/toolchain/581-verify-g1-contract-controlled-failure.stdout.txt`: Debug and Release both pass the good implementation and reject `constant42`, `special42`, and `wrong_exception` as controlled failures with exit 8 and no timeout.

## F1/F2 final checker audit

The previously reviewed F1/F2 constant-42 gap remains closed in the final snapshot:

- F1 Reference/Student checks cover `(20,1)`, `(0,0)`, and `(-2,3)`, so a constant `42` cannot pass.
- F2 Reference/Student checks cover `20`, `-1`, `0`, and `21`, so a constant `42` cannot pass.
- F2 now also has `delegation_check.cpp` and a spy provider, registered as a student test, requiring exactly one provider call and forwarding of the current input. This closes the later teaching review issue where copying the provider formula could pass without consuming the dependency boundary.

## Final quality/global state audit

- `Engineering_Study/references/quality-report.md` reports the same final matrix counts, G1/F2 closing evidence, G2 72-round measurement, and delivery manifest paths verified above.
- `Engineering_Study/README.md` points learners to implementation spec, coverage, quality report, and G2 measurement boundaries without presenting CTest existence as proof of completed learning.
- `LEARNCPP_GLOBAL_PLAN.md` records C01 as Windows-verified with explicit non-Windows boundary, and separates C01 completion from later G1/G2/global-course obligations.

## Validation commands and outputs used in this final pass

No long build or 72-round rerun was executed in this final reviewer pass. I used the retained final raw evidence and performed independent consistency checks:

- `Get-FileHash` for delivery manifests and final report files.
- Python JSON audit over `g2-formal-r1/report.json`, every retained round source/exe path, and all raw process JSON metadata. The local audit copy is retained under ignored build space: `Engineering_Study/exercises/build/final-technical-review/final-technical-review-audit.json`.
- XML/JSON inspection of `final-matrix-r3`, `c2-repeat-final.xml`, `g1-final-Debug.xml`, `g1-final-Release.xml`, and `581-verify-g1-contract-controlled-failure.stdout.txt`.
- Targeted `rg` inspection for final G1 contract, F2 delegation/spy checks, quality report, README, and global plan consistency.

I did not run Linux, ASan full matrix, or a second formal G2 sampling pass. Those remain explicit environment/range limits rather than hidden approvals.

Final recommendation: APPROVE the C01 technical implementation and data gate for the reviewed Windows/MSVC scope.
## Non-blocking option-semantics clarification

Severity: LOW, non-blocking.

`ENGINEERING_STUDY_BUILD_REFERENCE=OFF` is not currently specified as a student-only mode and is not promised to hide every `_reference` target or every `reference`-label test. The current public contract only gives the option default and requires Student/Reference target naming and labels; `BUILD_GUIDE.md` describes Student, Reference, and Observation as independent validation lanes. In the final snapshot, the option gates explicit optional/heavier reference paths in `C1_odr`, `E1_abi`, and `J1_package`, while lightweight reference/observation checks in A1/B1/C2/F1/F2/G1 remain available for comparison and normal validation.

This does not reopen the final technical/data approval. If the course later wants a strict student-only preset, the minimal follow-up is to document that as a new preset contract and then consistently gate all `reference` tests/targets under that new contract, followed by a fresh matrix rerun. For this delivery, keeping the existing reviewed preset validation surface is intentional and remains APPROVE.

