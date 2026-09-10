# U01 Linux Validation - u01-20260910-164550-8b2d0ea3

This validates the current U01 async logging snapshot on the dedicated WSL2 guest ext4 filesystem. It does not run benchmarks and does not approve the final frozen C08 tree.

## Snapshot And Dependencies

- Source snapshot: `/root/learncpp-c08-src/u01-20260910-164550-8b2d0ea3`
- Build root: `/root/learncpp-c08-builds/u01-20260910-164550-8b2d0ea3`
- Evidence root: `C08_Concurrency/references/validation/c08-revision/linux-preliminary/u01-20260910-164550-8b2d0ea3`
- Snapshot size: `368K`
- Manifest entries: `42`
- Manifest file SHA256: `236ba2747cd81df7e23806a27666f018fa55cc54b383e14c70ab7d8d732a3991`
- Leak check for hidden/build/binary/log files: clean (`excluded-leak-check.txt` is empty)
- fmt source: `/root/learncpp-c08-deps/fmt-12.1.0`
- spdlog source: `/root/learncpp-c08-deps/spdlog-1.17.0`

Dependency state:

```text
fmt_head=407c905e45ad75fc29bf0f9bb7c5c2fd3475976f
fmt_clean=1
spdlog_head=79524ddd08a4ec981b7fea76afd08ee05f83755d
spdlog_clean=1
```

The guest dependency copies were cloned with `git clone --no-hardlinks` from the local fixed source checkouts, then verified by HEAD and clean index. They do not depend on Windows build markers.

## Reference / Good / Bad Validation

From `results.env`:

```text
release-configure=0
release-build=0
release-ctest-list=0
release-ctest-focused=0
asan-configure=0
asan-build=0
asan-ctest-list=0
asan-ctest-focused=0
tsan-configure=0
tsan-build=0
tsan-ctest-list=0
tsan-ctest-focused=0
```

Focused tests run in each configuration:

```text
U01_async_logging_reference
U01_async_logging_good
U01_async_logging_bad_overflow_rejected
U01_async_logging_bad_flush_rejected
```

Results:

- Release with `g++`: 4/4 passed.
- ASan/UBSan with `clang++-18`: 4/4 passed.
- TSan with `clang++-18`: 4/4 passed.

The two negative tests passed because the expected bad implementations were rejected:

```text
Rejected the intended bad implementation: overrun counter
Rejected the intended bad implementation: queued flush runs during drain
```

## Student Initial State

The first attempt to run `U01_async_logging_student` in the reference configuration found zero tests because `CONCURRENCY_STUDY_TEST_STARTERS` was not enabled. That evidence is retained as `release-student-initial-reject.txt`, `asan-student-initial-reject.txt`, and `tsan-student-initial-reject.txt`, but it is not counted as student rejection evidence.

A separate student-only build enabled `CONCURRENCY_STUDY_BUILD_REFERENCE=OFF` and `CONCURRENCY_STUDY_TEST_STARTERS=ON`.

From `results.env`:

```text
release-student-configure=0
release-student-build=0
release-student-ctest-list=0
release-student-ctest-expected-fail=8
asan-student-configure=0
asan-student-build=0
asan-student-ctest-list=0
asan-student-ctest-expected-fail=8
tsan-student-configure=0
tsan-student-build=0
tsan-student-ctest-list=0
tsan-student-ctest-expected-fail=8
```

In all three configurations, `U01_async_logging_student` was registered and failed as expected:

```text
STARTER INCOMPLETE: U01 Part 1-4 未完成；未启动 async logger。
0% tests passed, 1 tests failed out of 1
EXIT: 8
```

This proves the starter does not produce a false pass before the student implementation is completed.

## Evidence Files

- Raw command output: `*.txt`
- Pure CTest test lists: `release-ctest-list.json`, `asan-ctest-list.json`, `tsan-ctest-list.json`, `release-student-ctest-list.json`, `asan-student-ctest-list.json`, `tsan-student-ctest-list.json`
- JUnit: `release-focused-junit.xml`, `asan-focused-junit.xml`, `tsan-focused-junit.xml`, `release-student-expected-fail-junit.xml`, `asan-student-expected-fail-junit.xml`, `tsan-student-expected-fail-junit.xml`
- Snapshot/dependency identity: `manifest.sha256`, `manifest-count.txt`, `manifest-file.sha256`, `snapshot-size.txt`, `deps-state.txt`

No `.log` files exist in this evidence directory.
