# C06 B01 性能规格与驱动非作者审查

## Verdict

BLOCK

B01 当前可以通过 `--check` 和小 smoke，但还不能进入正式 1 warmup + 5 sample 采样。阻断点都在正式采样门槛：pipeline 计时仍带计数插桩、checksum oracle 不是独立可靠基线、采样期间缺少 source/exe 改动复查。

本报告是非作者 verifier 审查，不是专用 architect/ralplan 批准。未修改作者源。

## Scope

- `C06_Ranges/exercises/B01_cost/**`
- `C06_Ranges/references/benchmarks/**`
- `C06_Ranges/exercises/CAPSTONE1_log_pipeline/src/reference/log_pipeline.hpp` and `include/log_pipeline_data.hpp` as read-only dependencies

## Evidence

Recorded with `C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py`:

- `references/validation/benchmark-review-configure.json`: PASS, isolated CMake configure exit 0.
- `references/validation/benchmark-review-build-release.json`: PASS, `B01_cost` Release target build exit 0.
- `references/validation/benchmark-review-ctest-release.json`: PASS, CTest exit 0 with `100% tests passed, 0 tests failed out of 1`.
- `references/validation/benchmark-review-check.json`: PASS, `B01_cost.exe --check` prints `B01_cost checks OK`.
- `references/validation/benchmark-review-smoke-run.json`: PASS, smoke driver exit 0.
- `references/validation/benchmark-review-smoke/report.json`: PASS, `formal=false`, 6 rounds, 5 summary entries, 18 raw files, source/exe/driver hashes present.
- `references/validation/benchmark-review-bad-n.json`: PASS, `--n 16385` exits 1 with `n must be in 1..16384`.
- `references/validation/benchmark-review-bad-rate.json`: PASS, `--rate 101` exits 1 with `hit rate must be in 0..100`.
- `references/validation/benchmark-review-bad-driver-samples.json`: PASS, `--samples 0` exits 2 with driver argument validation.

Static schedule check:

- `measure_b01.py::build_schedule(5, 1, 42, False)` produced 264 rounds: 44 warmup, 220 sample, no wrong per-key counts.
- Defaults match requested formal shape: scenarios use `N=256/8192`, rates `10/90`, seed `42`, 1 warmup and 5 samples.

## Blocking Findings

1. Pipeline timing path is still instrumented.

   `measure_b01.py` has `PIPELINE_VARIANTS = ("loop", "materialized", "reparse")` and schedules pipeline only with `instrument="timed"`; there is no counted/timed split for pipeline. In `B01_cost/main.cpp`, `run_pipeline` always uses `OperationCounts` and increments `parse_attempts` inside the timed region for all pipeline variants, then reports `parse_calls`.

   This violates the stated gate that counting and low-instrument timing are separated. It also conflicts with `B01_cost/README.md`, which says count and timed modes are separate and that count mode records `parse_calls`.

   Minimal fix: add a real pipeline `--instrument counted|timed` split. Keep correctness using the same algorithm body, but make the formal timed path avoid parse-count increments, and make counted runs record `parse_calls` without entering timing comparison.

2. The checksum oracle is not independent or stable enough for formal comparison.

   `measure_b01.py::attach_checksum_verdict` chooses the first PASS payload for `(case, scenario, phase)` as the baseline. Formal schedule order is shuffled by seed, so the oracle variant can differ by scenario/rep and can be any implementation. A buggy first variant could become the baseline and make correct later variants fail.

   For pipeline, `checksum_records` hashes only `level` and `user_id`; it does not include `timestamp` or `message`. That is not a complete oracle for `LogRecord` equality.

   Minimal fix: compute expected checksum/matches from an independent oracle for each scenario/seed, or run a named fixed oracle before shuffled variants and record it explicitly. For pipeline, include all semantically relevant record fields in the checksum or separately validate full record equality/counts against the oracle.

3. The driver records source/exe hashes only before the run.

   `measure_b01.py` records `source_sha256`, `capstone1_source_sha256`, `exe_sha256`, `driver_sha256`, and `process_runner_sha256` before executing the schedule. I found no post-run rehash or drift check. Formal sampling is long enough that source, driver, dependency, or exe drift during the run must invalidate the report instead of remaining invisible.

   Minimal fix: rehash the same artifacts after all rounds, save `*_sha256_after`, and set report status to FAIL if any source, dependency, driver, process runner, or exe hash changes.

## Passing Checks

- B01 depends on CAPSTONE1 read-only and includes the reference implementation through CMake include paths.
- `--check` uses the same `run_pipeline` / `run_index` functions as `--bench-one`, so correctness and benchmark entry share core code.
- Index benchmark separates `timed` and `counted` instruments.
- Index build and lookup windows are separated; unordered_map build includes `reserve` plus `emplace`, sorted_vector build includes copy plus sort, linear_vector build includes copy.
- Interface limit is enforced: `n` must be `1..16384`, queries equal `n`, rate must be `0..100`.
- Smoke driver saves raw stdout/stderr/json for each process and marks failed/timeout/cleanup-error samples invalid before summary comparison.
- Formal default schedule has the requested `N=256/8192`, `10/90`, fixed seed 42, 1 warmup, and 5 samples.

## Fingerprints

Reviewed source SHA256:

```text
714798108F882CD21053AC9E52484D9BB3D32C9325C7F9E7F028CF8147969B86  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\B01_cost\README.md
3A83D59D4DF12401595A4DE604B8599929A1CF782D698940FBFAB663BF2F1129  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\B01_cost\CMakeLists.txt
D620A10F3B55E1F09BBD5FDCC1CFE36932C6F66706C88B5B9C7B9A70B5D1940C  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\B01_cost\main.cpp
5F57D8FB1E2B074C7752321EBBB0B04CF2A62306D33C3CA2D92B23BA776CDEDC  F:\CPPTrain\LearnCPP\C06_Ranges\references\benchmarks\README.md
C8111A380C46CB4E62867FFB52A67305E0C795CC0404E87073F356B9B20E6F11  F:\CPPTrain\LearnCPP\C06_Ranges\references\benchmarks\scripts\measure_b01.py
4894D231B70C87DC70A3B8CC4BE01D479C6731262FC22AE8E140977F070B1EDA  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\src\reference\log_pipeline.hpp
8FE5CB0FF5C60CB4B0F28A4DBED9C235F3E8F104D258931112ED09DE37B93745  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\include\log_pipeline_data.hpp
```

## Gaps

- Formal sampling was intentionally not run.
- This review did not run the whole C06 course or ASan.

## Stop Condition

Do not start formal B01 sampling yet. Close the three blocking findings, rerun `--check`, smoke, controlled bad input, and this benchmark review before approving formal sampling.
