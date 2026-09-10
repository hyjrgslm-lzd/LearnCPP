# C06 Ranges usage slice review - 2026-09-10

Verdict: REQUEST CHANGES

Scope reviewed:

- Original usage chapters: `01-心智模型.md`, `02-模块A-视图工厂与惰性.md`, `03-模块B-基础适配器与管道.md`, `04-模块C1-结构适配器.md`, `05-模块C2-算法·投影·范围边界.md`, `06-模块D-C++23高阶视图与协程桥.md`, `07-结课项目1-数据管道与源码阅读.md`.
- Usage observation entries: `01_mental_model_warmup`, `A1_iota_view`, `A2_istream_view`, `A3_repeat_cartesian`, `B1_all_ref_owning`, `B2_filter_transform_degrade`, `B3_take_drop_closure`, `C1_1_join`, `C1_2_split_evolution`, `C1_3_common_reverse_elements`, `C2_1_projection`, `C2_2_dangling_borrowed`, `C2_3_ranges_to`, `D1_zip_adjacent_chunk`, `D2_chunk_by_join_with_asconst`, `D3_std_generator`.
- Public usage build helpers: `C06_Ranges/exercises/cmake/RangesSetup.cmake` and the above per-entry `CMakeLists.txt`.

## Stage 1 - Spec compliance

The implementation keeps the 16 usage entries as real observation programs, not Student skeletons. Each target CMake file calls `ranges_add_observation(... main.cpp)`, and the 16 `main.cpp` files contain live `check()`/`static_assert` coverage for their local concepts.

The slice still fails the "正文/解析/观察通过 must match the actual checked behavior" requirement. Several current README/reference-analysis statements claim checks or outputs that the corresponding `main.cpp` does not perform, and one chapter still contains a `static_assert` that is false for the shown example.

## Issues

### [HIGH] `join_view` example teaches a false `common_range` result

File: `C06_Ranges/04-模块C1-结构适配器.md:111`

Issue: The chapter says the `vector<vector<int>> | views::join` example has different `begin`/`end` types and asserts `!std::ranges::common_range<decltype(flat)>` at line 114. For the shown stored-inner-range case, current MSVC 19.51 accepts the opposite assertion: `std::ranges::common_range<decltype(flat)>` and `same_as<decltype(flat.begin()), decltype(flat.end())>`.

Why it matters: This is in the original teaching position for C1-1, so later "common fixes non-common ranges" material cannot repair it. Students following the snippet will either fail compilation or learn the wrong boundary: `join_view` is not inherently non-common; it depends on the base/inner range conditions.

Fix: Replace the assertion block with the conditional rule. For `vector<vector<int>>`, assert common. Use a separate non-common example, such as `iota(0) | filter(...) | take(...)`, for `views::common`, or construct a `join_view` case whose base/inner conditions really make it non-common.

Evidence: `C06_Ranges/references/validation/usage-review-probes/join_common_probe.cpp` builds successfully with the opposite assertion.

### [HIGH] D3 generator expected outputs contradict the real traversal

File: `C06_Ranges/06-模块D-C++23高阶视图与协程桥.md:785`

Issue: The chapter's tree sample visits `1 2 4 5 3` at line 776, then filters `v >= 3`, but the expected output comment says `3 4 5`. The actual preorder filter result for that sample is `4 5 3`.

File: `C06_Ranges/exercises/D3_std_generator/README.md:24`

Issue: The exercise README says the pipeline should produce `3 4 5`, while the actual observation program checks `4 3 5 6` in `C06_Ranges/exercises/D3_std_generator/main.cpp:60` against a different tree shape.

Why it matters: This is an observable behavior target. A student can implement a correct preorder generator and still be told by the exercise text to expect the wrong sequence.

Fix: Pick one tree shape for the chapter/README/program and make the preorder output plus filtered output match. The minimal fix is to update the comments/README to the actual `main.cpp` checks, or adjust `main.cpp` to match the documented tree if that is the intended teaching case.

### [MEDIUM] Several exercise "当前程序"解析 claims do not match the checked program

File: `C06_Ranges/exercises/B1_all_ref_owning/README.md:98`

Issue: It says the current program mutates the original vector to prove `ref_view` observes external state and uses accumulation to prove `owning_view` owns elements. The actual program only checks the `ref_view` pointer and `owning_view` size (`main.cpp:15`, `main.cpp:20`).

File: `C06_Ranges/exercises/B3_take_drop_closure/README.md:123`

Issue: It says the current program checks `drop_while` output. The actual program has no `drop_while` observation; it checks `take`, `drop`, `take_while`, list `drop`, and closure composition (`main.cpp:18`, `main.cpp:19`, `main.cpp:29`, `main.cpp:33`).

File: `C06_Ranges/exercises/C2_3_ranges_to/README.md:115`

Issue: It says the current program materializes string splits into `vector<string>`. The actual program materializes odd ints, list, string from char iota, `MinimalContainer`, and `from_range`; there is no split-token case (`main.cpp:25`, `main.cpp:35`, `main.cpp:38`, `main.cpp:41`, `main.cpp:44`).

File: `C06_Ranges/exercises/D1_zip_adjacent_chunk/README.md:83`

Issue: It says the current program checks an `adjacent<3>` window sum. The actual program checks tuple size and window count, but not a window sum (`main.cpp:26`, `main.cpp:27`).

File: `C06_Ranges/exercises/D2_chunk_by_join_with_asconst/README.md:83`

Issue: It says the current program verifies "连续递增分组"; the actual program uses `chunk_by(std::equal_to<>{})` and verifies equal runs (`main.cpp:9`, `main.cpp:15`).

Why it matters: The migration to observation-style entries is structurally correct, but the reference-analysis layer overstates or misstates what passed. That breaks the promised distinction between observed PASS and unverified extension work.

Fix: For each README, either update "当前程序" to exactly describe the existing checks, or add the missing checks when they are part of the required observation. Keep extension-only work clearly labeled as extension/unverified.

### [MEDIUM] D3 README states the wrong generator start event in one local explanation

File: `C06_Ranges/exercises/D3_std_generator/README.md:43`

Issue: It says the coroutine body waits until the first `++it` to run to the first `co_yield`. The chapter and program correctly show first `begin()` starts execution to the first yield (`06-模块D-C++23高阶视图与协程桥.md:882`, `D3_std_generator/main.cpp:49`).

Why it matters: `std::generator::begin()` versus iterator increment is a core lifecycle boundary for this exercise.

Fix: Change the README sentence to "第一次 `begin()` 启动协程并运行到第一个 `co_yield`，之后每次 `++it` 推进到下一个 `co_yield`."

### [LOW] Stale generated line-count footer remains in module D

File: `C06_Ranges/06-模块D-C++23高阶视图与协程桥.md:957`

Issue: The footer says "全文 580 行", but the current file has 957 lines.

Fix: Delete the footer or update it only if this project intentionally keeps generated line-count metadata. Deleting is better; it adds no teaching value.

## Stage 2 - Code quality and verification

No security issue found in the reviewed C++ usage programs. Pattern scan over the 16 `main.cpp` files found no broad `catch`, no silent skip branch, no feature macro bypass, and no hardcoded secret pattern. The only "fallback" hit is the intended `ranges::to` insertion-path observation in `C2_3_ranges_to/main.cpp:42`, not a masking workaround.

Compiler diagnostics substitute: no `lsp_diagnostics` tool was exposed in this lane, so I used the project compiler as the concrete diagnostics source. I configured, built, and ran CTest for all 16 usage entries in both Release and Debug using out-of-tree build dirs under `C06_Ranges/references/validation/usage-review-build-20260910`; all 32 runs returned configure/build/ctest exit code 0. Raw JSON: `C06_Ranges/references/validation/usage-review-build-20260910/results.json`.

## Recommendation

REQUEST CHANGES

The 16 observation programs are build-clean and mostly useful, but the current documentation/README layer still contains false behavior claims in the exact usage slice under review. Fix the cited text or add the cited missing checks, then rerun the same 16-entry Debug/Release matrix.
