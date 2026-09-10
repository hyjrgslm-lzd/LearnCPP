# C08 Linux Preliminary Validation - baseline-20260910-162842-fc6bb723

This run validates a fresh compact guest ext4 snapshot after the source fixes for:

- no-hook overloads for hookable queue APIs;
- `CMAKE_CXX_SCAN_FOR_MODULES=OFF` for this non-module course;
- Linux Clang link with `atomic`.

It is a portability discovery run, not final course approval. Windows-side full dependency validation and final frozen-source Linux matrix were not run here.

## Snapshot

- Source snapshot: `/root/learncpp-c08-src/baseline-20260910-162842-fc6bb723`
- Build root: `/root/learncpp-c08-builds/baseline-20260910-162842-fc6bb723`
- Evidence root: `C08_Concurrency/references/validation/c08-revision/linux-preliminary/baseline-20260910-162842-fc6bb723`
- Snapshot size: `4.8M`
- Manifest entries: `495`
- Manifest file SHA256: `45eb21a32d6ececdd8f9eedb72547a55502cd610a90775c4c6d033576c21daf0`
- Leak check for hidden/build/binary/log files: clean (`excluded-leak-check.txt` is empty)

The snapshot includes C08 course/source files plus root planning docs and selected cross-course README link targets. It excludes hidden directories, build directories, `_deps`, binary artifacts, archives, and log/cache files.

## Matrix Results

From `results.env`:

```text
linux-core-configure=0
linux-core-build=0
linux-core-ctest-list=0
linux-core-ctest=0
linux-debug-configure=0
linux-debug-build=0
linux-debug-ctest-list=0
linux-debug-ctest=0
linux-asan-configure=0
linux-asan-build=0
linux-asan-ctest-list=0
linux-asan-ctest=0
linux-tsan-configure=0
linux-tsan-build=1
linux-tsan-partial-ctest=1
```

### Passing Configurations

`linux-core`:

```text
100% tests passed, 0 tests failed out of 64
```

`linux-debug`:

```text
100% tests passed, 0 tests failed out of 64
```

`linux-asan` (`CONCURRENCY_STUDY_SANITIZER=address`, Linux ASan+UBSan):

```text
100% tests passed, 0 tests failed out of 64
```

In all three passing configurations, these tests were skipped by capability/platform return code, not counted as pass:

```text
F01_thread_attributes_reference
F02_hazard_pointer_batches_reference
F03_std_senders_reference
F03_std_hazard_pointer_reference
F03_std_rcu_reference
M2_execution_bridge_reference
N1_numa_placement_reference
```

`runtime_materials`, `U01_async_logging`, and `F03_native_facilities` were excluded in the ctest command for this preliminary run because related work was still in parallel/freeze pending.

### TSan Status

`linux-tsan` configure passed, so the prior Clang scanner failure is resolved.

`linux-tsan` full build failed while linking `runtime_scheduling_test`:

```text
multiple definition of `operator new(unsigned long)'
multiple definition of `operator new[](unsigned long)'
multiple definition of `operator delete(void*)'
multiple definition of `operator delete[](void*)'
multiple definition of `operator delete(void*, unsigned long)'
multiple definition of `operator delete[](void*, unsigned long)'
```

Source location:

```text
C08_Concurrency/exercises/runtime_tests/scheduling_test.cpp:11-20
```

The file defines global allocation operators for failure injection; TSan also provides new/delete interceptors, so the target cannot link under TSan as currently written.

After the failed full build, a partial TSan ctest run excluded missing/unlinked runtime tests plus the same freeze-pending tests. Result:

```text
97% tests passed, 2 tests failed out of 61
```

Failures:

```text
B3_call_once_reference: Timeout at 120.10 sec
F3_seqcst_fence_reference: ThreadSanitizer data race
```

TSan report source location:

```text
C08_Concurrency/exercises/F3_seqcst_fence/solution.cpp:57 write data = 42
C08_Concurrency/exercises/F3_seqcst_fence/solution.cpp:64 read observed = data
```

This is real sanitizer evidence for the current test body under TSan, not an environment calibration failure.

## Evidence Files

- Commands and raw output: `*-configure.txt`, `*-build.txt`, `*-ctest.txt`
- Pure CTest test lists: `linux-core-ctest-list.json`, `linux-debug-ctest-list.json`, `linux-asan-ctest-list.json`, `linux-tsan-ctest-list.json`
- JUnit: `linux-core-junit.xml`, `linux-debug-junit.xml`, `linux-asan-junit.xml`, `linux-tsan-partial-junit.xml`
- Snapshot identity: `manifest.sha256`, `manifest-count.txt`, `manifest-file.sha256`, `snapshot-size.txt`
- Rename map: `log-rename-map.txt`

No `.log` files remain in this run directory.
