# C08 queue formal measurement review

Verdict: APPROVE.

Scope reviewed: `references/measurements/c08-revision-final/{queue-storage,queue-batch,queue-spsc,queue-mpsc,queue-ms}/`, `references/validation/c08-revision/measurement-window.json`, `references/validation/c08-revision/final-measurements-run.json`, `exercises/tools/run_benchmarks.py`, `exercises/benchmarks/CMakeLists.txt`, generated `queue_bench`/`queue_diagnostics` project files, and the measurement README. Independent recalculation artifact: `measurement-review-recalc.json`.

## Evidence

- Raw run count matches the claim: 5 groups, 48 benchmark process records, 40 formal runs, 8 warmups, and 12 correctness prechecks. All `checks[]` and `runs[]` have `status=PASS`, `exit_code=0`, and no timeout.
- Every group records `seed=42`, `warmups=1`, `samples=5`. Python `random.Random(42)` replay matches the stored variant/repetition/warmup order. The initial PowerShell/.NET RNG cross-check is intentionally ignored because it is not Python's shuffle algorithm.
- `samples.csv` rows match exactly the formal, non-warmup rows reconstructed from each `run.json`; precheck/warmup/failure rows are not mixed into statistics.
- Each case has five formal samples, one result per repetition, `completed=100003`, and a single consistent `details` string within the case. Recomputed median/min/max/sample-stdev match `run.json` and the README table.
- Source and executable binding is internally stable: all five `run.json` files record `source_sha256=065f5f48a8e19725f42c3a75a90f81a02469c110175726296b0907fbdcced5ec` and `executable_sha256=3d59cca9f22b5735d9598ed1582331c5aed8147d60b7de51f6001fdb04e4f986`. `Get-FileHash` on the current `queue_bench.exe` matches the recorded executable hash. The current runner-scope source digest is now `5f1b93c85811750a94fa26f13015e4db4e598b31a11b4fd1a47b8c8a30e0751a`, which is expected after later validation/report files were added and is not treated as a sample defect.
- Timed binary binding is clean: `queue_bench` Release `CL.command.1.tlog` has `/O2 /Ob2 /DNDEBUG`, `CONCURRENCY_STUDY_SANITIZER:STRING=none`, no `CS_QUEUE_DIAGNOSTICS`, no sanitizer flag, and no `/fp:fast`. `queue_diagnostics` is a separate target and is the one compiled with `CS_QUEUE_DIAGNOSTICS=1`.
- Semantic grouping is consistent with the documented boundaries: storage compares `mutex`/`ring` under P=3/C=4/capacity=64/batch=1; `batch` uses P=3/C=4/capacity=64/batch=8; SPSC uses P=1/C=1; MPSC/MPMC use P=3/C=1; MS uses capacity=0 and records unbounded allocation/HP-lock cost.
- README conclusions stay inside the evidence: they preserve negative samples, avoid total-time hardware bottleneck claims, avoid cache-miss/kernel attribution, keep batch/MS out of bounded queue rankings, and state that 20260908 historical samples are a different machine/snapshot. The old `final-20260908` directory still contains 17 groups.

## Recomputed Statistics

| group | variant | median | min | max | stdev |
|---|---|---:|---:|---:|---:|
| queue-batch | batch | 9.4782 | 7.7606 | 11.0680 | 1.2272 |
| queue-mpsc | mpmc | 8.0734 | 7.2294 | 9.3230 | 0.8739 |
| queue-mpsc | mpsc | 7.7030 | 6.7382 | 16.7129 | 4.1749 |
| queue-ms | ms | 50.3672 | 43.9556 | 58.4877 | 6.2716 |
| queue-spsc | spsc | 1.2527 | 1.2044 | 1.5345 | 0.1307 |
| queue-spsc | spsc-cached | 1.3730 | 1.2705 | 1.8191 | 0.2213 |
| queue-storage | mutex | 23.4159 | 16.0466 | 27.7353 | 5.5194 |
| queue-storage | ring | 21.8832 | 19.0096 | 27.0526 | 3.0145 |

## Gaps

- This review did not rerun the long benchmark suite; it verifies the sealed raw records, runner contract, generated build metadata, and current executable hash.
- OS background load, CPU frequency, affinity, hardware counters, NUMA placement, tail latency, fairness, and profiler attribution remain unverified, matching the README's stated limits.
- The recorded performance applies to the measured source snapshot and executable hash. Later documentation or source edits require a new measurement snapshot before making new performance claims.

