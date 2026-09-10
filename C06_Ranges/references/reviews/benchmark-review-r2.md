# C06 B01 性能规格与驱动 r2 非作者复验

## Verdict

APPROVE

B01 r2 关闭了上一轮 3 个正式采样门槛 BLOCK。可以放行正式采样准备，但本次没有运行 formal。

本报告是非作者 verifier 复验，不是专用 architect/ralplan 批准。未修改作者源。

## Scope

- `C06_Ranges/exercises/B01_cost/**`
- `C06_Ranges/references/benchmarks/**`
- CAP1 四路径 collector 新增无计数 overload 的默认行为回归

## Evidence

Recorded with `C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py`:

- `references/validation/benchmark-review-r2-configure.json`: PASS, isolated B01 configure exit 0.
- `references/validation/benchmark-review-r2-build-release.json`: PASS, B01 Release build exit 0.
- `references/validation/benchmark-review-r2-check.json`: PASS, `B01_cost.exe --check` prints `B01_cost checks OK`.
- `references/validation/benchmark-review-r2-ctest-release.json`: PASS, B01 CTest `1/1`.
- `references/validation/benchmark-review-r2-smoke-run.json`: PASS, smoke driver exit 0.
- `references/validation/benchmark-review-r2-drift-self-check.json`: PASS, synthetic drift self-check prints `B01 drift self-check OK`.
- `references/validation/benchmark-review-r2-bad-n.json`: PASS, `--n 16385` exits 1.
- `references/validation/benchmark-review-r2-bad-instrument.json`: PASS, invalid instrument exits 1.
- `references/validation/benchmark-review-r2-cap1-configure.json`: PASS, CAP1 configure exit 0.
- `references/validation/benchmark-review-r2-cap1-build-release.json`: PASS, CAP1 Release build exit 0.
- `references/validation/benchmark-review-r2-cap1-ctest-release.json`: PASS, CAP1 CTest `3/3`.
- `references/validation/benchmark-review-r2-cap1-build-student.json`: PASS, CAP1 Student target builds.
- `references/validation/benchmark-review-r2-cap1-student-exit1.json`: PASS, CAP1 Student initial state exits 1 with `valid row parses`.

Smoke report check:

- `references/validation/benchmark-review-r2-smoke/report.json`: `status=PASS`, `formal=false`, 7 rounds, 6 summary entries, 21 raw files.
- Pipeline timed rounds have `parse_calls=0`.
- Pipeline counted loop round has `parse_calls=256`.
- Pipeline and index payloads have `checksum == oracle_checksum`; pipeline also matches oracle valid/error counts; index matches oracle match count.
- `hash_drift` is empty, and `source/exe/driver/process_runner` after hashes equal before hashes.

Formal schedule static check:

- `build_schedule(5, 1, 42, False)` produces 336 rounds: 56 warmup and 280 sample.
- Both pipeline and index schedule `timed` and `counted`.
- All formal keys have 1 warmup and 5 samples.

## Previous Blocker Recheck

1. Timed path parse-count instrumentation: closed.

   B01 now dispatches `run_pipeline_impl<false>` for `instrument=timed` and calls CAP1 no-count overloads. The smoke report proves timed pipeline `parse_calls=0`, while counted loop reports `256`. CAP1 default counted overloads still pass the sample checker.

2. Independent oracle: closed.

   Pipeline input construction now stores `expected_records`, checksum includes `timestamp`, `level`, `user_id`, and `message`, and `same_records` checks full record equality. Index uses `index_oracle` as a linear independent oracle. The driver validates payload oracle fields instead of treating the first shuffled variant as baseline.

3. End-of-run hash drift: closed.

   Driver records before hashes, rehashes after all rounds, stores `*_sha256_after`, writes `hash_drift`, and makes report status FAIL if drift is non-empty. `--drift-self-check` verifies the drift helper catches a synthetic changed driver hash.

4. CAP1 no-count overload regression: closed.

   Reference/good/bad/student now expose no-count overloads without changing default counted APIs. CAP1 Release CTest remains `3/3`, and Student still fails safely with exit 1.

## Fingerprints

Reviewed source SHA256:

```text
6115352626B172DF4BB20131B925AB1840D41C4C48368FC02D9478024DCE2DA1  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\B01_cost\README.md
3A83D59D4DF12401595A4DE604B8599929A1CF782D698940FBFAB663BF2F1129  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\B01_cost\CMakeLists.txt
CCD8B22CA9423BD1501D8D93BB030E46B05A64E3DC8626380E117022BA4F7F88  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\B01_cost\main.cpp
955AB303BF299BFF9E125A5988D12D006FFB84D8671328079B7276710712CE5A  F:\CPPTrain\LearnCPP\C06_Ranges\references\benchmarks\README.md
B9B4518B70C66553CD0DCC2B3F0D0542372D74BE518099EA39CE4CF39ADF52D8  F:\CPPTrain\LearnCPP\C06_Ranges\references\benchmarks\scripts\measure_b01.py
84E7B5114D4703B24381D21491BBF23F00C3154F349D79A0EE846385000C1F33  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\main.cpp
E743935111206DFF3E292AA4BF48591256ADACD5043624E7F2BAF7AB511D2B23  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\src\reference\log_pipeline.hpp
DE4FC02B195FC552E9AF0FF8614E4069F967198B6E6AF93F38236ED482644EE5  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\validation\good\log_pipeline.hpp
5446C055F76CDB13EEC14E0277C5135332E6AD11A6903E8F337B315DEA568D21  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\validation\bad\log_pipeline.hpp
775A4F272B64B51A1CD6268C357A0332C926B0CA25D91F09130E0D49F7A2D492  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\src\student\log_pipeline.hpp
8FE5CB0FF5C60CB4B0F28A4DBED9C235F3E8F104D258931112ED09DE37B93745  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\include\log_pipeline_data.hpp
```

## Gaps

- Formal sampling was intentionally not run.
- ASan and whole-course C06 validation were not run.

## Stop Condition

Formal B01 sampling may proceed after coordinating a quiet machine window. The formal run must keep every raw sample, reject timeout/error/cleanup/hash-drift groups, and not infer root cause from timing alone.
