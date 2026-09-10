# C08 async logging timeout review r2 2026-09-10

Review role: non-author r2 verification for the logging timeout fix.

Relation to r1: this review verifies closure of `logging-review.md` without overwriting that report. Earlier U01 teaching, checker, good/bad, source pin, and default-off checks were already reviewed there and are not repeated here.

Reviewed files and artifacts:

- `C08_Concurrency/exercises/U01_async_logging/CMakeLists.txt`
  - SHA256: `5BC628C25ADBA8C340B818BF2D668772E7CD03D2BD8D3F236D2CA0EF2DB91524`
- `C08_Concurrency/exercises/U01_async_logging/expect_failure.cmake`
  - SHA256: `2DF0C4590A23DD56C6DC0FA2BAA3CCB669C36B3A044C8B130F8215FF45404525`
- `C08_Concurrency/exercises/U01_async_logging/validation/evidence-20260910-timeout-r2.json`
  - SHA256: `87FA7B1E14CE0A4F819DF2AA24A70D1752F378143C5B40F1CA5B28DFD90B76BF`
- r1 review: `C08_Concurrency/references/validation/c08-revision/reviews/logging-review.md`
  - SHA256: `9DE19BCAA7C1183CBA6B29F23879BE28C73D7AF8D00BE216F4871CF8DDD88963`

## Verdict

APPROVE.

The r1 blocking timeout issue is closed. U01 custom validation CTest properties now derive from `CS_TEST_TIMEOUT`, and the negative wrapper receives the same process timeout with a 5-second outer CTest cleanup margin.

## Closure check

The r1 blocker was: U01 good/bad validation tests used fixed `TIMEOUT 30`, while ASan should inherit the 120-second sanitizer test window; `expect_failure.cmake` also had a fixed inner `TIMEOUT 20`.

r2 fixes this in two places:

- `U01_async_logging/CMakeLists.txt` computes `CS_U01_EXPECT_FAILURE_CTEST_TIMEOUT` as `${CS_TEST_TIMEOUT} + 5`, passes `-DPROCESS_TIMEOUT=${CS_TEST_TIMEOUT}` into both bad wrappers, sets `U01_async_logging_good` to `TIMEOUT ${CS_TEST_TIMEOUT}`, and sets both bad wrapper tests to `TIMEOUT ${CS_U01_EXPECT_FAILURE_CTEST_TIMEOUT}`.
- `expect_failure.cmake` defaults `PROCESS_TIMEOUT` to `30` only when not provided, then uses `TIMEOUT ${PROCESS_TIMEOUT}` for the bad executable.

Generated CTest properties confirm the intended values:

- Release tree:
  - `U01_async_logging_good`: `TIMEOUT "30"`
  - bad wrapper process: `PROCESS_TIMEOUT=30`
  - bad wrapper CTest timeout: `TIMEOUT "35"`
- ASan tree:
  - `U01_async_logging_reference`: `TIMEOUT "120"`
  - `U01_async_logging_good`: `TIMEOUT "120"`
  - bad wrapper process: `PROCESS_TIMEOUT=120`
  - bad wrapper CTest timeout: `TIMEOUT "125"`

This closes the exact r1 failure mode where sanitizer preset/config said 120 but U01-specific per-test properties still capped tests at 30.

## Fresh reviewer evidence

Commands run from `F:\CPPTrain\LearnCPP`:

```text
cmake --build C08_Concurrency/exercises/build/c08-logging-author --target U01_async_logging_checked --config Release
ctest --test-dir C08_Concurrency/exercises/build/c08-logging-author -C Release -R U01_async_logging --output-on-failure
```

Result: PASS. Release U01 tests: `4/4` passed; total real time `0.60 sec`.

```text
cmake --build C08_Concurrency/exercises/build/c08-logging-asan --target U01_async_logging_checked --config RelWithDebInfo
ctest --test-dir C08_Concurrency/exercises/build/c08-logging-asan -C RelWithDebInfo -R U01_async_logging --output-on-failure
```

Result: PASS. ASan U01 tests: `4/4` passed; total real time `0.73 sec`.

Generated property inspection:

```text
rg -n 'U01_async_logging_(reference|good|bad_overflow_rejected|bad_flush_rejected)|TIMEOUT "(30|35|120|125)"|PROCESS_TIMEOUT=(30|120)' `
  C08_Concurrency/exercises/build/c08-logging-author/U01_async_logging/CTestTestfile.cmake `
  C08_Concurrency/exercises/build/c08-logging-asan/U01_async_logging/CTestTestfile.cmake
```

Result: PASS. The generated files contain the Release `30/35` and ASan `120/125` values listed above.

## Boundary

This r2 review only covers the timeout repair. It does not re-approve unrelated U01 teaching logic from scratch and does not edit any reviewed file. The earlier `logging-review.md` passing checks remain the basis for the non-timeout parts.

Stop condition: r1 REVISE timeout item is closed; report written only here.
