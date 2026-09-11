# C09 S1 样章非作者门禁审查

结论：APPROVE。

审查时间：2026-09-11。
审查人角色：native code-reviewer，非实现作者。
工作区 HEAD：75028ff2d312a4f1047e7bafd6cf4101f771a2ec。
审查范围：共享 `lazy_task.hpp` 的 `await_resume` RAII 修复；新增 `runtime_tests/await_resume_exception_test.cpp` 与 CMake 注册；模块 G “七点一”样章；G3 README 的 `lazy_task<void>` 返回口径修正。

未修改被审源码或正文。本次只新增/写入：

- `C09_Coroutines/references/validation/c09-refresh/reviews/s1-review.md`
- `C09_Coroutines/references/validation/c09-refresh/reviews/s1-review-evidence/**`

## 规格符合性

`C09_Coroutines/references/implementation-spec.md` 要求 S1 先以 G3 与共享 `lazy_task` 为样章，完成“原失败 → 最小修复 → 复跑 → 非作者审查”，通过后再批量深化其他主题。当前变更符合该边界：它只处理结果提取期间的 frame 所有权，没有把 Student TODO 自动填完，也没有引入新依赖、CI、系统安装、提交或推送。

教学口径也符合规格：C09 继续主讲协程协议、控制流和生命周期；G3 README 把完成通知、结果消费和销毁拆开说明，并明确本题复用的 `coroutine_study::sync_wait(lazy_task<void>&&)` 返回 `void`，没有把 sender consumer 的 optional/tuple 协议混入本题。

## 技术审查

通过。

被修复点在 `C09_Coroutines/exercises/include/coroutine_study/lazy_task.hpp:164`：

```cpp
lazy_task owned{std::exchange(callee, {})};
```

随后 `p.exception`、`!p.result` 和 `return std::move(*p.result)` 都发生在 `owned` 仍存活期间。若结果移动构造抛异常，栈展开会销毁 `owned`，从而销毁 child coroutine frame。若正常返回，返回对象先构造完成，再销毁 `owned`。`callee` 已被置空，awaiter 析构不会二次 destroy，单次消费语义保留。

没有看到 root-cause guard 禁止的遮蔽式 fallback：没有吞异常、静默默认值、旁路分支或“best effort”路径。修复直接恢复 primary contract：结果提取期间必须仍有 frame owner。

`lazy_task<void>::awaiter::await_resume()` 未改动是合理的：void 分支没有用户 `T` 的 throwing move 窗口；其异常路径在 rethrow 前显式 `h.destroy()`，正常路径也 destroy。

## 实验审查

通过。

新增测试 `C09_Coroutines/exercises/runtime_tests/await_resume_exception_test.cpp` 覆盖两个必要路径：

- 正常路径：`sync_wait(consume_value()) == 7` 且 `tracked_value::alive == 0`。
- 异常路径：结果移动构造抛 `runtime_error("result move failed")`，要求 `caught=1` 且 `alive_after_unwind=0`。

测试注册在 `C09_Coroutines/exercises/runtime_tests/CMakeLists.txt:26`，继承 runtime helper 的 `reference;runtime` label 和 30 秒 timeout。

主代理保存的原始失败证据：

- `C09_Coroutines/references/validation/c09-refresh/s1/s1-original-failure-20260911T021240884313Z.json`
- command: `C09_Coroutines/exercises/build/c09-plan-baseline/runtime_tests/Release/runtime_await_resume_exception_test.exe`
- exit_code: 1
- stdout: `caught=1 alive_after_unwind=1`
- verdict: FAIL

主代理保存的修复后回归证据：

- `C09_Coroutines/references/validation/c09-refresh/s1/s1-after-regression-20260911T021513558180Z.json`
- command: `ctest --test-dir C09_Coroutines/exercises/build/c09-plan-baseline -C Release -R "runtime_await_resume_exception_test|G3_sync_wait_impl_reference|runtime_lazy_task_reference_test|runtime_sync_wait_reference_test|runtime_sync_completion_reference_test" --output-on-failure`
- exit_code: 0
- result: `100% tests passed, 0 tests failed out of 5`

独立复验命令与输出已保存：

- `C09_Coroutines/references/validation/c09-refresh/reviews/s1-review-evidence/wsl-gpp-command.txt`
- `C09_Coroutines/references/validation/c09-refresh/reviews/s1-review-evidence/wsl-gpp-meta.json`
- `C09_Coroutines/references/validation/c09-refresh/reviews/s1-review-evidence/wsl-gpp-stdout.txt`
- `C09_Coroutines/references/validation/c09-refresh/reviews/s1-review-evidence/wsl-gpp-stderr.txt`

独立命令：

```text
wsl.exe bash -lc "cd /mnt/f/CPPTrain/LearnCPP && mkdir -p C09_Coroutines/references/validation/c09-refresh/reviews/s1-review-evidence/wsl-gpp && g++ -std=c++20 -Wall -Wextra -pedantic -I C09_Coroutines/exercises/include C09_Coroutines/exercises/runtime_tests/await_resume_exception_test.cpp -o C09_Coroutines/references/validation/c09-refresh/reviews/s1-review-evidence/wsl-gpp/runtime_await_resume_exception_test && C09_Coroutines/references/validation/c09-refresh/reviews/s1-review-evidence/wsl-gpp/runtime_await_resume_exception_test"
```

独立结果：

```text
exit_code=0
caught=1 alive_after_unwind=0
```

独立复验绑定的当前文件 hash：

```text
lazy_task_sha256=30a0ad1b5da689f52f24eb10ea57751cafcf6a37564c5731983aaa58269be624
test_sha256=e530448b72cbcabcc1744809c6b8a186a6f9da4f344d8e73e8bd2ad914eb41ba
```

## source-before 核对

`C09_Coroutines/references/validation/c09-refresh/s1/source-before.json` 记录：

```text
source_commit=75028ff2d312a4f1047e7bafd6cf4101f771a2ec
before lazy_task.hpp sha256=45ecbd1bac6a9e368c8c72586afadd5d7b940d98f220398d779c32a8fa1f3998
before await_resume_exception_test.cpp sha256=c728e5af9f6368da273458765f36a859cd0c62d12d846dffdadc02c4e7763c92
```

当前 `lazy_task.hpp` hash 与 before hash 不同，符合本次修复预期。当前 `await_resume_exception_test.cpp` hash 与 before hash 也不同；LF/CRLF 归一化后仍不匹配。该点不阻断 S1，因为当前测试源码已被独立审查并用 WSL g++ 复验通过，且原始失败 JSON 保留了失败程序输出。但最终质量报告不要声称 source-before 中的 test 与当前 test 字节一致；应绑定当前 review evidence 的 hash。

hash 复核输出保存于：

- `C09_Coroutines/references/validation/c09-refresh/reviews/s1-review-evidence/hash-check-output.txt`

## 教学审查

通过。

模块 G 新增“七点一”没有只给修复结论，而是按教学要求给出：

1. 先修：父子 task、final suspend、移动构造与 RAII。
2. 失败阶段定位：异常发生在调用方 `await_resume()` 结果提取期间，不是 child body 内。
3. 所有权图：旧实现先清空 awaiter 句柄，再移动结果，异常跳过 destroy。
4. 最小实验：Release 下用 `tracked_value::alive` 证明泄漏，不依赖 Sanitizer 或耗时推断。
5. 最小修复：复用 `lazy_task` owner，不新增抽象。
6. 反例提醒：catch 中补 destroy 和把 move 标成 noexcept 都不是通用根因修复。
7. 验证命令：给出 build/ctest 复现入口。

这满足样章门禁：机制、标准行为边界、最小反例、可运行观察和答案解析都具备。

## 限制与后续要求

- 本次只审 S1 样章，不代表 C09 37 单元全课通过。
- 未运行 lsp_diagnostics/ast_grep_search；当前工具面没有这两个工具。替代验证为 MSVC CTest 证据回读和独立 WSL g++ C++20 编译运行。
- 后续批量修改可以继续，但每个共享 runtime 变更仍要保持“原失败证据 → 根因修复 → 新鲜复验 → 非作者审查”的链条。

## Verdict

APPROVE。S1 样章门禁通过，可作为后续单元批量深化的样章基线。
