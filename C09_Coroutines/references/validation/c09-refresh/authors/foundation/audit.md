# C09 foundation slice author audit

范围：00-05 正文关联的 P1/P2/A1-A3/B1-B3/C1-C3/Capstone1，各题目录含本地 CMake。未修改公共 include、顶层 CMake、AddExercise 或 references 总表。

## 12 单元归类

| 单元 | 分类 | 处理 |
| --- | --- | --- |
| P1_future_basics | 无需改 | 观察型 future 结果通道，保留 Starter/Reference。 |
| P2_generator_basics | 无需改 | 观察型 generator 启动/推进/析构，保留 Starter/Reference。 |
| A1_first_generator | 无需改 | 已有 Part、答案解析和 Reference 检查，保留。 |
| A2_co_return_lazy_task | 无需改 | 已有 lazy task 路径、答案解析和 Reference 检查，保留。 |
| A3_co_await_future | 实现/检查补强 | Starter 增加 not-ready future 行为检查，要求 worker 路径恢复；未完成同步占位现在 exit=1。 |
| B1_recursive_generator | 实现/检查补强 | Starter 收集两个 generator 输出并比对同步 inorder 基准；只 yield root 的占位现在 exit=1。 |
| B2_task_sequential | 实现/检查补强 | Starter 检查父 task 的完整格式化结果；简写占位现在 exit=1。 |
| B3_callback_to_awaiter | 检查补强 | Starter 检查 callback 结果必须通过 await_resume 回到协程体并得到 36；当前实现通过。 |
| C1_stop_token_cancel | 检查补强 | Starter 检查部分进度 stopped 路径和 throwing cancellation 路径；当前实现通过。 |
| C2_when_all_when_any | 实现/检查补强 | Starter 检查 when_all 槽位、并发耗时、200ms/400ms when_any winner；串行/只等第一分支占位现在 exit=1。 |
| C3_async_scope | 检查补强 | Starter 检查 scope 析构后 5 个 task 都完成，且不是明显串行 drain；当前实现通过。 |
| Capstone1_async_crawler | 实现/检查补强+bugfix | Starter 检查 parser、汇总报告、并发/stop 行为；Reference 修复 move 后 catch 读取 URL 的错误路径。 |

## 变更摘要

- A3/B1/B2/B3/C1/C2/C3/Capstone1 的 `CMakeLists.txt` 添加 `STUDENT_TEST` 参数；公共 AddExercise 合入后由 `COROUTINE_STUDY_TEST_STARTERS=ON` 注册同名 starter 测试。
- A3/B1/B2/B3/C1/C2/C3/Capstone1 的 `main.cpp` 增加本地行为检查和 `main() try/catch`，失败打印 `student check failed: ...` 并返回 1。
- A3/B1/B2/B3/C1/C2/C3/Capstone1 的 `README.md` 增加 `Student 检查` 表，列出 Part -> 操作 -> 本地检查；文字/观察题不伪装成自动测试。
- Capstone1 `solution.cpp` 在 `fetch_into` 进入 `co_await fetch_one(... std::move(rec) ...)` 前保存 URL；catch 路径使用保存值。新增 `fail_next_submit` 注入，验证 submit 失败时仍保留 URL。

## Capstone1 原失败证据

`capstone1-move-url-repro-before.json`：独立 C++ 最小复现编译通过，运行 `runExit=7`，stdout 为 `observed_url=''`。这证明原形状在 move 后 catch 读 `rec.url` 会丢失错误 URL。

## 验证

- 全量 `configure_windows.py --build C09_Coroutines/exercises/build/c09-author-foundation`：exit=1。阻断来自未分配范围 `F3_halo_diagnose` 缺 `main.cpp`，已记录到 `configure_windows-full-blocked.json`，未修改 F3。
- slice configure：`cmake -S .../foundation/slice-source -B C09_Coroutines/exercises/build/c09-author-foundation-slice -G "Visual Studio 18 2026" -A x64` -> exit=0。
- slice build：24 个目标（12 starter + 12 reference）Release 构建 -> exit=0。
- Reference CTest：`ctest --test-dir C09_Coroutines/exercises/build/c09-author-foundation-slice -C Release -L reference --output-on-failure` -> 12/12 passed。
- Starter direct runs：见 `starter-runs.json`。预期失败 A3/B1/B2/C2/Capstone1 均 exit=1 且有诊断；B3/C1/C3 exit=0。
- diff hygiene：`git diff --check -- <foundation paths>` -> exit=0，仅 Git 行尾转换 warning。

## 实际 target 列表

Starter：P1_future_basics、P2_generator_basics、A1_first_generator、A2_co_return_lazy_task、A3_co_await_future、B1_recursive_generator、B2_task_sequential、B3_callback_to_awaiter、C1_stop_token_cancel、C2_when_all_when_any、C3_async_scope、Capstone1_async_crawler。

Reference：上述每个 target 的 `_reference`。

## 未测 / 交给主代理

- 公共 `AddExercise.cmake` 尚未在本分支显示 `STUDENT_TEST` 解析和 starter `add_test` 注册；本轮用 direct run 验证 starter 行为。主代理合入后需跑 `COROUTINE_STUDY_TEST_STARTERS=ON` 的 CTest。
- 全量 configure 被 F3 阻断；需要 F3 所有者补齐或给顶层/本地 CMake 明确跳过策略后再用指定 `c09-author-foundation` 全量目录复跑。
- 本切片没有补学生 good/bad 独立源码文件；当前代表性 bad 由未完成 starter 直接运行证明，good 由独立 Reference CTest 证明。

## R2 修正：正确性不用耗时阈值

- C2 `main.cpp` 改为 `start_gate/gated_awaitable`：child 进入挂起点时登记 handle，只有 expected 个 child 都登记后才释放。串行 `sync_wait` 会在第一个 child 发现仍在 caller thread 上执行并有限失败，当前诊断为 `Part 2: when_all/when_any must drive child tasks outside the caller thread, not serial sync_wait`。
- C3 `main.cpp` 改为 5 个 gated task：全部从 owned worker 路径进入挂起点后才释放，scope 离开后检查 `started=5 completed=5`。移除了 elapsed 阈值。
- C2 README 已说明 timeout/耗时只保留为观察输出，不作为正确性判据。C3 README 已说明 PASS 只代表 Part 1/2 gate 检查通过。

## R2 代表 good/bad 接线证明

| 类型 | 证据 | 结论 |
| --- | --- | --- |
| bad: A3 | `starter-runs-r2.json` 中 `A3_co_await_future exitCode=1` | 同步等待占位被 worker resume 检查拒绝。 |
| bad: B1 | `B1_recursive_generator exitCode=1` | 只 yield root 的占位被 inorder 序列检查拒绝。 |
| bad: B2 | `B2_task_sequential exitCode=1` | 简写字符串被父 task 结果检查拒绝。 |
| bad: C2 | `C2_when_all_when_any exitCode=1` | 串行/只等第一分支在 gated child 处有限失败，不靠 sleep timeout。 |
| bad: Capstone1 | `Capstone1_async_crawler exitCode=1` | parser 分数占位被 parser probe 拒绝。 |
| good: B3 covered parts | `B3_callback_to_awaiter exitCode=0` | 证明 Part 1/2/4 的值流和 owner 收束接线生效；不声明 Part 3 生产级同步完成窗口已实现。 |
| good: C1 covered parts | `C1_stop_token_cancel exitCode=0` | 证明 Part 1/2/3 的 stopped/throwing 通道接线生效；CPU 长循环检查点仍为观察解析。 |
| good: C3 covered parts | `C3_async_scope exitCode=0` | 证明 Part 1/2 的 gate/scope 等齐接线生效；悬挂引用和 detach 仍为观察解析。 |
| reference good | `ctest -L reference` 12/12 passed | Reference 独立通过，未被 Student include。独立 full-good 学生实现留给后续非作者补，不复制 reference 冒充。 |

## R2 B3/C1/C3 逐 Part 真实覆盖口径

| 单元 | Part | 当前 Student 运行结论 |
| --- | --- | --- |
| B3 | Part 1 回调 API | PASS：`async_add` 被 awaiter 调用，worker_group 收束后台线程。 |
| B3 | Part 2 awaiter 三方法 | PASS：值必须经 `await_resume` 进入协程体，最终为 36。 |
| B3 | Part 3 同步完成窗口 | OBSERVE：`async_add_immediate` 只用于分析；未把 ready/return-false 生产级处理计为 PASS。 |
| B3 | Part 4 结果同步与异常 | PARTIAL PASS：值流已检查；异常扩展仍是进阶。 |
| C1 | Part 1 stop state | PASS：`request_stop` 后结果小于总批次。 |
| C1 | Part 2 检查点 | PASS：等待前/后检查能产生部分进度。 |
| C1 | Part 3 stopped vs throwing | PASS：预取消 throwing 版本经 `sync_wait` 抛 `task_cancelled`。 |
| C1 | CPU 长循环不检查 token | OBSERVE：README/正文解释，不运行 CPU 长循环自动测试。 |
| C3 | Part 1 spawn 与 join | PASS：5 个 gated task 全部从 worker 路径到达 gate。 |
| C3 | Part 2 析构等齐 | PASS：scope 离开后 `started=5 completed=5`。 |
| C3 | Part 3 悬挂引用时间线 | OBSERVE：不运行 UB，不计入实现 PASS。 |
| C3 | Part 4 detach 对照 | OBSERVE：不把非确定日志当自动测试。 |

## R2 正文 00-05 反向说明

| 正文 | 反向检查 | 处理 |
| --- | --- | --- |
| 00 预备知识 | P1/P2 的 future/generator 先修在正文和 README 双向链接；观察型任务无需改成失败 checker。 | 无需补强。 |
| 01 心智模型 | A/B/C 前的 frame、promise、awaiter、owner 心智模型已有承接；本轮未发现下游必须偷看 Reference 的新概念缺口。 | 无需补强。 |
| 02 模块 A | A3 的 future 适配、ready/suspend/resume/exception 在正文与 README 已讲；本轮补的是 Student 行为检查，不是正文缺讲。 | 无需补强。 |
| 03 模块 B | B1/B2/B3 的 generator/task/callback-to-awaiter 均有 Part 与答案解析；本轮补检查与 PASS 口径。 | 无需补强。 |
| 04 模块 C | 原正文把耗时差异称为可观察证据，容易被误作正确性门禁。 | 已补一句：耗时只作观察，Starter 用受控 gate 证明 fan-out。 |
| 05 Capstone1 | 拓扑、fetch、parser、when_all_fetch、stop、aggregate 已覆盖；本轮补 Student 检查和 Reference 错误 URL 修复。 | 无需大段重写。 |

## R2 验证追加

- `cmake --build C09_Coroutines/exercises/build/c09-author-foundation-slice --config Release --target C2_when_all_when_any C3_async_scope C2_when_all_when_any_reference C3_async_scope_reference` -> exit=0。
- `starter-runs-c2-c3-gated-r2.json`：C2 starter exit=1，诊断为 caller-thread serial；C3 starter exit=0，输出 `started=5 completed=5`。
- `starter-runs-r2.json`：A3/B1/B2/C2/Capstone1 预期失败且有诊断；B3/C1/C3 covered parts 通过。
- `ctest --test-dir C09_Coroutines/exercises/build/c09-author-foundation-slice -C Release -R "C2_when_all_when_any_reference|C3_async_scope_reference" --output-on-failure` -> 2/2 passed。

## R3 修正：gate 不拒绝合法单线程 fan-out

R2 的 C2/C3 gate 曾把 `std::this_thread::get_id() != driver_thread` 当作正确性条件。这会误拒绝合法的单线程事件循环：协程组合要求先 fan-out 再收束，不要求不同 OS 线程。R3 删除这类线程 identity 断言，保留 `started/released/completed` 受控 gate 作为正确性证据。

- C2 `main.cpp`：删除 `driver_thread` 状态和 `get_id()` 检查；`when_all` / `when_any` 的待实现主体改为明确 `throw std::logic_error("TODO[必做 ...]")` 的有限安全 stub。当前 starter 会真实调用学生路径并失败，不会靠串行占位挂死，也不会用无条件返回码替代 checker。
- C3 `main.cpp`：删除 `driver_thread` 状态和 `get_id()` 检查；scope gate 只要求 5 个 task 都到达挂起点并在析构收束后完成。
- C2 README 和 `04-模块C-取消与组合.md`：`50/150/300ms`、`200/400ms` 仅作为单独观察输入；当前门禁不使用 300/500ms 耗时阈值。正文明确 fan-out 可以由多线程实现，也可以由单线程事件循环实现。
- C3 README：明确 gate 不要求不同线程。

### R3 good/bad 证据

| 类型 | 证据 | 结论 |
| --- | --- | --- |
| good: same-thread fan-out | `r3-gate-same-thread-good-bad.json`：独立 C++ 小程序 `runExit=0`，stdout 含 `good_same_thread=true` | 同一 driver thread 顺序启动 3 个 child，前两个挂起，第三个到达后释放全部 waiter；gate 不误拒绝单线程 fan-out。 |
| bad: missing slot | 同一记录 stdout 含 `bad_missing_slot_detected=true` | 只启动 2/3 个 child 时，`released=false completed=0`，有限可观察，不靠 timeout 当通过。 |
| bad: wrong result | 同一记录 stdout 含 `bad_wrong_result_detected=true` | 错误结果槽可由 tuple 比较有限检出。 |
| student C2 current stub | `starter-runs-r3.json`：`C2_when_all_when_any exit=1`，诊断 `TODO[必做 1]...` | 默认未实现路径有限失败；后续学生实现同一 `when_all/when_any` 路径后可转绿。 |
| student C3 current covered parts | `starter-runs-r3.json`：`C3_async_scope exit=0`，输出 `started=5 completed=5` | 当前覆盖 Part 1/2 的 spawn/析构收束，不声明观察型 UB/detach 为自动 PASS。 |

### R3 复验

- `cmake --build C09_Coroutines/exercises/build/c09-author-foundation-slice --config Release --target C2_when_all_when_any C3_async_scope C2_when_all_when_any_reference C3_async_scope_reference` -> exit=0。
- `starter-runs-r3.json`：C2 starter exit=1，有限 TODO 诊断；C3 starter exit=0，`started=5 completed=5`。
- `ctest --test-dir C09_Coroutines/exercises/build/c09-author-foundation-slice -C Release -R "C2_when_all_when_any_reference|C3_async_scope_reference" --output-on-failure` -> 2/2 passed。
- `ctest --test-dir C09_Coroutines/exercises/build/c09-author-foundation-slice -C Release -L reference --output-on-failure` -> 12/12 passed。
