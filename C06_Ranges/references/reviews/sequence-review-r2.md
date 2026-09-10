# C06 基础序列 slice 独立复验 r2

## Verdict

APPROVE

本次只复验上一轮唯一 BLOCK：`L05_dynamic_array` checker 的超大 `reserve` 异常类型契约。未重新审查 C06 其它未冻结批次，也不冒称专用 architect / ralplan 审查完成。

## Closed Blocker

上一轮问题：`C06_Ranges/exercises/L05_dynamic_array/main.cpp` 的 `check_limits_and_alignment()` 同时接受 `std::length_error` 和 `std::bad_alloc`，会放过没有容量预检、直接尝试巨大分配的实现。

当前状态：已关闭。`C06_Ranges/exercises/L05_dynamic_array/main.cpp:206` 现在只 catch `std::length_error`；复查未发现 `bad_alloc` 接受路径。该行为与 `C06_Ranges/chapters/04-dynamic-array.md:73`、`C06_Ranges/exercises/L05_dynamic_array/README.md:15` 的契约一致。

## Evidence

作者 r2 证据读取：

- `foundation-sequence-l05-ctest-release-r2.json`: PASS, `0 tests failed out of 3`.
- `foundation-sequence-l05-ctest-debug-r2.json`: PASS, `0 tests failed out of 3`.
- `foundation-sequence-l05-student-exit1-release-r2.json`: expected exit 1, `check failed: reserve grows capacity`.
- `foundation-sequence-l05-student-exit1-debug-r2.json`: expected exit 1, `check failed: reserve grows capacity`.

独立复验使用既有 `build-review`，顺序执行 MSBuild，避免同一 build tree 并发：

- `sequence-review-r2-l05-build-release.json`: PASS.
- `sequence-review-r2-l05-ctest-release.json`: PASS, `0 tests failed out of 3`.
- `sequence-review-r2-l05-build-student-release.json`: PASS.
- `sequence-review-r2-l05-student-exit1-release.json`: PASS, expected exit 1 with `check failed: reserve grows capacity`.
- `sequence-review-r2-l05-build-debug.json`: PASS.
- `sequence-review-r2-l05-ctest-debug.json`: PASS, `0 tests failed out of 3`.
- `sequence-review-r2-l05-build-student-debug.json`: PASS.
- `sequence-review-r2-l05-student-exit1-debug.json`: PASS, expected exit 1 with `check failed: reserve grows capacity`.

No dedicated `lsp_diagnostics` tool is exposed in this lane; MSVC Debug/Release builds are the available compile/type diagnostic gate.

## Recommendation

APPROVE

旧 BLOCK 已按最小修复关闭，受影响 checker 和 L05 Debug/Release 行为复验通过。
