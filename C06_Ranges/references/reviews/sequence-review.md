# C06 基础序列 slice 独立审查

## Verdict

BLOCK

这是 `foundation_sequence` 冻结范围的独立技术/教学 slice review，覆盖：

- `chapters/01-complexity-contracts.md`
- `chapters/02-sequence-containers.md`
- `chapters/03-iterators-invalidation.md`
- `chapters/04-dynamic-array.md`
- `exercises/L01_complexity/**`
- `exercises/L02_sequence_containers/**`
- `exercises/L05_dynamic_array/**`
- `exercises/cmake/RangesSetup.cmake` 的相关四路径练习接线

本结论不冒称专用 architect / ralplan 审查完成，也不覆盖 C06 其它未冻结批次。

## Blocking Issue

[MEDIUM] `C06_Ranges/exercises/L05_dynamic_array/main.cpp:204`

Issue: `check_limits_and_alignment()` 的超大 `reserve` 检查把 `std::bad_alloc` 当成满足容量契约。`C06_Ranges/chapters/04-dynamic-array.md:73` 和 `C06_Ranges/exercises/L05_dynamic_array/README.md:15` 明确要求 `n > max_size()` 抛 `std::length_error`，并且不要尝试分配溢出大小。当前 checker 在 `main.cpp:206-210` 同时接受 `length_error` 和 `bad_alloc`，会放过没有安全容量预检、直接尝试巨大分配的实现。

Root cause: checker 把“拒绝且状态不变”降级成“任何分配失败也算拒绝”，掩盖了被教学正文要求的主契约：先判定容量边界，再抛 `length_error`。

Impact: Student 或后续坏实现可以省掉 `max_size()` / 溢出预检，只靠 allocator 失败通过该检查；这削弱了本 slice 明确要求的安全容量边界和异常语义验证。

Minimum fix: 删除 `catch (const std::bad_alloc&) { length_error = true; }` 这条接受路径，让测试只接受 `std::length_error`。如要保留内存耗尽测试，应另加独立场景，不能和 `n > max_size()` 契约合并。

## Passing Evidence

Fresh reviewer runs used isolated `build-review` directories and wrote raw records under `C06_Ranges/references/validation/sequence-review-*`.

- L01 configure/build/CTest Debug+Release: PASS. CTest reports `0 tests failed out of 1`.
- L02 configure/build/CTest Debug+Release: PASS. CTest reports `0 tests failed out of 1`.
- L05 configure/build/CTest Debug+Release: PASS after sequential MSBuild rerun. CTest reports `0 tests failed out of 3`.
- L05 Student Release build: PASS.
- L05 Student Release execution: expected exit 1 with `check failed: reserve grows capacity`.

Two earlier L05 build records, `sequence-review-l05-build-release.json` and `sequence-review-l05-build-debug.json`, are reviewer-side false negatives from concurrent MSBuild use of the same `build-review` tree: both exit 1 without compiler diagnostics. Sequential reruns are recorded as `sequence-review-l05-build-release-r3.json` and `sequence-review-l05-build-debug-r2.json`.

## Review Notes

- Stage 1 spec compliance was checked before code quality. L01/L02/L05 match the intended foundation sequence shape: observation exercises stay bounded; L05 uses independent Student/Reference/good/bad include paths; unfinished Student is not registered as a default passing test.
- L05 Reference implementation covers raw storage, over-aligned allocation via allocator, move-only ownership transfer, copy fallback for throwing move types, borrowed span invalidation without dereferencing stale storage, empty `pop_back`, and exception rollback.
- `validation/good/dynamic_array.hpp` is independent by implementation strategy (`std::vector<T>` wrapper) and does not include the Reference header.
- `validation/bad/dynamic_array.hpp` is a real early-commit bug and is rejected by the configured `BAD_DIAGNOSTIC`.
- No dedicated `lsp_diagnostics` tool is exposed in this lane; MSVC Debug/Release builds were used as the available type/compile diagnostic gate.

## Recommendation

REQUEST CHANGES

修复 L05 checker 的超大 `reserve` 异常类型契约后，重跑 L05 Debug/Release CTest 和 Student exit1；L01/L02 不需要返工。
