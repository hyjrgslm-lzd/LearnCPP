# C08 Linux Preliminary Validation - baseline-20260910-161541-e469c27a

This is a portability discovery run on a guest ext4 snapshot. It is not final course approval.

## Snapshot

- Source snapshot: `/root/learncpp-c08-src/baseline-20260910-161541-e469c27a`
- Build root: `/root/learncpp-c08-builds/baseline-20260910-161541-e469c27a`
- Evidence root: `C08_Concurrency/references/validation/c08-revision/linux-preliminary/baseline-20260910-161541-e469c27a`
- Snapshot size: `615M`
- Manifest entries: `24428`
- Manifest file SHA256: `0ddf36cbbff0d986fd502b435a8c123e118c5caa9e080e9d25c8afb17c101d97`
- Copy command evidence: `robocopy.txt`, `paths.env`

Excluded during copy: `.git`, `.omx`, `.codex`, `.agents`, `_deps`, `build`, `build-*`, `out`, `.vs`, `.cache`, and common binary/log/cache files.

## Commands Run

Main script:

```powershell
wsl -d LearnCPP-C08-Ubuntu-24.04 --user root -- bash /mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/linux-preliminary/baseline-20260910-161541-e469c27a/run-linux-prelim.sh
```

Preset results from `results.env`:

```text
linux-core-configure=0
linux-core-build=1
linux-debug-configure=0
linux-debug-build=1
linux-asan-configure=1
linux-tsan-configure=1
```

`ctest` and JUnit output were not produced because no preset reached a complete build.

## Findings

### 1. GCC linux-core/linux-debug build failure

Both `linux-core` and `linux-debug` configure successfully with `g++ 13.3.0`, then fail compiling `Capstone2_lockfree_queue`.

Representative error:

```text
C08_Concurrency/exercises/include/concurrency_study/queue_checks.hpp:253:29: error: no matching function for call to 'cs::queue_lab::sequence_ring<long unsigned int, false, unsigned char>::try_push(std::size_t&)'
C08_Concurrency/exercises/include/concurrency_study/queue_versions.hpp:183:10: note: candidate: 'template<class Hook> bool cs::queue_lab::sequence_ring<T, SingleConsumer, Counter>::try_push(const T&, Hook)'
```

Likely root cause: `sequence_ring::try_push` is a function template whose `Hook` parameter only appears in the default function argument. Calling `try_push(value)` cannot deduce `Hook` on GCC.

Temporary guest-copy diagnostic added a non-template forwarding overload for `sequence_ring::try_push(value)`. Build then advanced to the same pattern in `ms_queue::try_pop(value)`:

```text
C08_Concurrency/exercises/include/concurrency_study/queue_linked.hpp:100:10: note: candidate: 'template<class Hook> bool cs::queue_lab::ms_queue<T>::try_pop(T&, Hook)'
```

This diagnostic changed only `/root/learncpp-c08-src/baseline-20260910-161541-e469c27a-diagnose`; it was not counted as source validation.

### 2. Clang linux-asan/linux-tsan configure failure

Both `linux-asan` and `linux-tsan` fail in `find_package(Threads REQUIRED)` before any course target builds.

Representative CMake log:

```text
FAILED: CMakeFiles/cmTC_43b71.dir/CheckForPthreads.cxx.o.ddi
"CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS-NOTFOUND" -format=p1689 -- /usr/bin/clang++-18 ...
/bin/sh: 1: CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS-NOTFOUND: not found
```

Manual toolchain check:

```text
clang++-18 -std=c++23 -pthread .../cxx23_probe.cpp
sum=4
```

So `clang++-18` accepts `-pthread`; the configure failure is CMake/Ninja scanner setup, not missing pthread support.

Temporary no-source-change configure with `-DCMAKE_CXX_SCAN_FOR_MODULES=OFF` succeeded:

```text
-- Found Threads: TRUE
-- Configuring done
```

The follow-up ASan no-scan build then failed at link:

```text
undefined reference to `__atomic_is_lock_free'
```

That points to a Linux Clang/libstdc++ link requirement, likely `atomic`, for targets that call `std::atomic::is_lock_free`.

## Evidence Files

- `manifest.sha256`
- `manifest-count.txt`
- `manifest-file.sha256`
- `snapshot-size.txt`
- `linux-core-configure.txt`
- `linux-core-build.txt`
- `linux-debug-configure.txt`
- `linux-debug-build.txt`
- `linux-asan-configure.txt`
- `linux-tsan-configure.txt`
- `linux-asan-noscan-configure.txt`
- `linux-asan-noscan-build.txt`
- `run-linux-prelim.sh`
- `log-rename-map.txt`

## Next Required Source Fixes Before Final Linux Matrix

1. Add ordinary overloads or otherwise make no-hook calls valid for hookable queue APIs:
   - `sequence_ring::try_push(value)`
   - `ms_queue::try_pop(value)`
   - audit sibling hookable APIs for the same template-default pattern.
2. Make Linux Clang presets disable C++ module dependency scanning unless `clang-scan-deps` is available.
3. Link `atomic` for Linux Clang/libstdc++ targets that use `std::atomic::is_lock_free`, then rerun ASan/UBSan and TSan from a fresh snapshot.
