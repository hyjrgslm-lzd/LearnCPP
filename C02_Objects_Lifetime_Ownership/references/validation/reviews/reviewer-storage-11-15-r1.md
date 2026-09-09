# C02 Independent Review - Storage 11-15 r1

Reviewer: C02 independent teaching/code reviewer
Scope: final current storage batch: chapters 11-15; exercises `L11_layout`, `L12_storage`, `L13_aliasing`, `L14_ub`, `P1_object_buffer`; author report `Core_Study/references/validation/author-storage-report.md`.
Verdict: REQUEST_CHANGES
Date: 2026-09-09

## Source binding

Evidence root: `Core_Study/references/validation/reviews/c02-independent-evidence/storage-11-15-r1b/`

The first capture prefix `storage-11-15-r1/` is preserved but invalid as test evidence: my PowerShell helper used a parameter named `$args`, so the commands ran without their intended arguments and only produced tool usage output. I corrected the helper, did not overwrite that prefix, and reran under `storage-11-15-r1b/`.

Key source hashes in `source-hashes.json`:

- `author-storage-report.md`: `8AA110177B5C927122EB97EF79EEA9D594A8573B0441994567CE4CA6FFB3E790`
- `implementation-spec.md`: `2EB445384AE32FD123C95ABC455D7921E347CB625B252AF9BD5776023A4FC24A`
- `CONTENT_REFACTORING_GUIDE.md`: `6ED24D43193C9E9F6D111BB1FED9378D35B61E6336A9F6D92A88CDFAD9D89BF3`
- `chapters/12-storage-and-object-creation.md`: `C52EE5724F2468B5614C2467BD9E923FACF4BFEC2EB91459E752646EF78B4719`
- `chapters/14-undefined-behavior-and-optimization.md`: `F48730DA2E5660AA8A9BC8B0BB539FEF9E43F6C865FC3798AEFF70A098BE7C6E`
- `chapters/15-object-buffer.md`: `8720C71F1737502324513C24612E949CA80197F27F9E605B213C28FDDC0C0F32`
- `L12_storage/CMakeLists.txt`: `0304335D12D76FD788F200B18199E46C3E7FD51FB17E620EF1161EAC9B767B36`
- `L12_storage/checks/expect_failure.cmake`: `DC2EF83636017660B7810CB26E6C03D32E910D868CDBE3FAD36A3E785E63AFBD`
- `P1_object_buffer/src/reference/object_buffer.hpp`: `11031B05BAF698837601183CD021AB24ED11014639E2F49053F0ACDBA9EE0861`
- `P1_object_buffer/checks/run_checks.cpp`: `D0BD3E1650E1365C6521146807B49071362C0D136FEE970BDF6D171FB7C63092`

## Stage 1 - spec / teaching compliance

11-15正文不是符号目录。11 从对象字节/地址、成员填充、数组边界、表示、standard-layout/trivially-copyable、EBO 推到后续 `object_buffer` 的 `view()` 边界。12 先建立显式构造模型，再分开讲 C++20 implicit object creation、`allocator<T>::allocate(n)` 的 `T[n]` 数组对象边界、`start_lifetime_as` 能力状态和 union 活跃成员。13 区分合法类型访问、字节观察、`bit_cast`/`memcpy`、透明替换、`launder` 和 provenance/DR 边界。14 把 UB/IFNDR/implementation-defined/unspecified/erroneous behavior 与诊断工具分层，并把前沿 probe 的 PASS/SKIP/诊断后接受分开。15 从固定 `object_buffer<T>` API、状态不变量、强保证顺序、失败注入、借用失效和下游回访连续推到 P1 checker。

Root 指出的“`operator new` 无对象”过度说法已经闭合：12 章现在明确“分配函数不调用构造函数”，同时说明 implicit-lifetime 类型可由特定操作隐式开始生命期；`allocator<T>::allocate(n)` 开始的是数组对象边界，不是元素对象生命期（`12-storage-and-object-creation.md:5-10`, `:46-57`, `:131-139`）。P1 ASan 问题也被正确分类为 Clang 22.1.3 + Windows ASan + MSVC EH rethrow 窄路径；15 章没有把 C++ `throw;` 改写成非法规则（`15-object-buffer.md:131-151`）。

## Stage 2 - code / validation review

### HIGH 1. L12 negative test does not actually match the fixed diagnostic; CMake splits `EXPECTED_TEXT` at spaces

File: `Core_Study/exercises/L12_storage/CMakeLists.txt:40`

Issue: The negative wrapper is intended to require the exact diagnostic `slot must report engaged after successful construct`, but the `add_test` call passes it as unquoted list arguments:

```cmake
-DEXPECTED_TEXT=slot must report engaged after successful construct
```

CTest verbose output from my rerun shows the actual command is split:

```text
"-DEXPECTED_TEXT=slot" "must" "report" "engaged" "after" "successful" "construct"
```

Therefore `expect_failure.cmake` receives only `EXPECTED_TEXT=slot`. I verified the weakness with an isolated fake command `fake-slot-unrelated.cmd` that prints `slot unrelated failure from fake` and exits 7; running the same wrapper with `-DEXPECTED_TEXT=slot` exits 0. Evidence: `l12-negative-verbose.stdout.txt`, `l12-negative-weak-match-probe.json`, `l12-negative-weak-match-probe.stdout.txt`.

Why this blocks: The batch spec requires public bad validation to reject representative wrong implementations with the target checker diagnostic, not accept arbitrary failure text. Matching only `slot` is too broad and can accept unrelated failures from the executable or harness.

Fix: Quote the full CMake definition as one argument, for example:

```cmake
"-DEXPECTED_TEXT=slot must report engaged after successful construct"
```

Then rerun `ctest -R L12_storage_validation_bad_noop_check -V` and add a narrow negative probe proving unrelated `slot ...` text no longer passes.

## Non-blocking reviewed items

### L11 layout/alignment/representation: no blocking issue found

The observation target only asserts standard guarantees and prints local ABI facts. It avoids dereferencing one-past pointers and does not treat local size/alignment as portable truth. Debug and Release leaf CTest passed.

### L12 storage semantics: implementation body is consistent, aside from the diagnostic wiring issue above

`storage_slot<T>` commits `engaged_` only after `std::construct_at` succeeds and keeps failed construction empty. `destroy()` is guarded and `noexcept` under the nothrow-destructor precondition. Student placeholder builds and fails at the first actual contract check, not through a ready flag or self-report. The `start_lifetime_as` target executed on my MSVC route and printed `L12_start_lifetime_as OK macro=202207`.

### L13 alias/launder/provenance: no blocking issue found

The exercise runs only legal `bit_cast`, `memcpy` into an already-live target, byte/representation observation, and construct/destroy/reconstruct with `std::launder`. The chapter text keeps provenance/invalid-pointer/lifetime-end DR as review/compile-only boundaries and does not claim runtime proof where none exists.

### L14 behavior/frontier/unsafe boundaries: no blocking issue found in scoped review

Default L14 registers only the safe observation target. With `CORE_STUDY_ENABLE_FRONTIER=ON`, fixed compile-only probes are real CTest consumers. My MSVC frontier rerun produced baseline PASS, P2287 SKIP, P2748 SKIP after diagnostic, P2953 SKIP, and provenance compile-only PASS. The P2287 source uses the requested indirect member designator shape `Derived{.base = 1, .member = 2}`, not `.Base`. The unsafe ASan target is gated by both `CORE_STUDY_ENABLE_ASAN` and `CORE_STUDY_ENABLE_UNSAFE_DEMOS`; I reviewed the CMake consumer and wrapper that require nonzero exit plus `heap-use-after-free` and `asan_uaf_probe.cpp`. I did not rerun the Clang ASan unsafe lane in this teaching/code pass; author raw evidence and the separate verifier lane remain the matrix evidence source.

### P1 `object_buffer<T>`: no blocking issue found in scoped review

The frozen Reference keeps the fixed API and uses `std::unique_ptr` rollback guards. `reserve` allocates into a local pointer, migrates old elements, releases cleanup only after success, then destroys old storage and commits capacity. `push_back(T value)` constructs the new tail before migrating old elements on growth, destroys exactly the new tail and successful prefix on migration failure, and commits `size_` only after success. The checker covers no-growth, self-view append, copy failure, new-tail failure, old-copy failure during growth, over-aligned values, move-only borrowing transfer, move assignment invalidation, self move, and throwing-move-only compile rejection. Debug/Release CTest and direct Reference runs passed; public bad variants were rejected by expected-failure wrappers.

## Validation performed

All command stdout/stderr and JSON summaries are under `Core_Study/references/validation/reviews/c02-independent-evidence/storage-11-15-r1b/`.

- MSVC Debug leaf configure/build/CTest:
  - L11/L12/L13/L14/P1 all exit 0. Summary: `msvc-debug-summary.json`.
- MSVC Release leaf configure/build/CTest:
  - L11/L12/L13/L14/P1 all exit 0. Summary: `msvc-release-summary.json`.
- Student isolation:
  - L12 Student reference-off build exit 0; CTest exit 8 with `check failed: slot must report engaged after successful construct`.
  - P1 Student reference-off build exit 0; CTest exit 8 with `check failed: reserve grows capacity`.
- Direct runs:
  - `L11_layout_observation.exe`, `L12_storage_reference.exe`, `L12_storage_start_lifetime_as.exe`, `L13_aliasing_observation.exe`, `L14_ub_observation.exe`, `P1_object_buffer_reference.exe` all exit 0.
- Negative / capability checks:
  - L12 bad noop CTest currently passes its wrapper, but the wrapper only matches `slot`; this is the blocking issue above.
  - P1 bad noop and bad early commit wrappers pass by matching their target diagnostics.
  - P1 throwing-move-only compile rejection wrapper passes.
  - L14 MSVC frontier CTest exits 0 with expected PASS/SKIP split.
- Sensitive scan:
  - `sensitive-scan.json` records `rg` exit 1 over the scoped docs/source for common token/secret patterns, with no matches.
- Author evidence index:
  - `author-json-index.txt` records raw author JSON statuses. Preserved intermediate FAIL/SKIP classifications remain visible; I did not rewrite them as new pass evidence.

Tooling limitation: no `lsp_diagnostics` tool is exposed in this worker session. I used MSVC Debug/Release builds, CTest, direct executables, CMake verbose output, isolated fake wrapper probe, source hashing, and `rg` scans.

## Recommendation

REQUEST_CHANGES for storage 11-15 r1 because L12 negative validation does not enforce the fixed diagnostic. After quoting the full `EXPECTED_TEXT` and proving the fake unrelated `slot` failure is rejected, the remaining reviewed teaching/code items are suitable for closeout without repeating the full matrix.
