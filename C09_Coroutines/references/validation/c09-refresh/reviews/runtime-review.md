# C09 公共 coroutine runtime 非作者审查

结论：REQUEST CHANGES。

审查时间：2026-09-11。
审查人角色：native code-reviewer，非实现作者。
工作区 HEAD：75028ff2d312a4f1047e7bafd6cf4101f771a2ec。
当前 `mini_ref/mini.hpp` SHA256：f6e35941658c1e1e5702b75b3721b8e2a6dbc94ba824a9d74168ff52edff3da4。
审查范围：`C09_Coroutines/exercises/include/coroutine_study/**`、`C09_Coroutines/exercises/Capstone5_mini_corolib/reference/include/mini_ref/mini.hpp`、`stdexec_awaitable.hpp`、现有 runtime / mini_ref reference tests 与消费者索引。

只写入本审查文件和证据目录：

- `C09_Coroutines/references/validation/c09-refresh/reviews/runtime-review.md`
- `C09_Coroutines/references/validation/c09-refresh/reviews/runtime-review-evidence/**`

未修改被审代码、正文或其他作者正在编辑的文件。

## Stage 1：规格符合性

`implementation-spec.md` 要求公共 runtime 覆盖同步/跨线程/await_suspend 返回前完成、early destruction、失败收束、一次性恢复；错误和取消不能靠总 CTest 绿灯替代。当前 `lazy_task` 的结果移动异常修复已通过 S1，但本轮发现公共 runtime 仍有生命周期和取消收束阻断，不能进入公共 runtime 完成声明。

## Stage 2：代码质量与生命周期发现

### [HIGH] 栈上 `sync_wait_state` 可能在 `notify_one()` 前被等待线程销毁

File: `C09_Coroutines/exercises/include/coroutine_study/lazy_task.hpp:42`

问题：`notify_sync_wait()` 在 `:45-47` 持锁设置 `done=true` 后释放锁，随后 `:48` 才调用 `state.cv.notify_one()`。`state` 是 `sync_wait()` 的栈变量，见 `:294` / `:314`。等待线程在 `:301-303` 或 `:321-323` 用 predicate 等待；C++ condition_variable 允许 spurious wake。若等待线程在通知方释放锁与调用 `notify_one()` 之间 spurious wake，看到 `done=true` 后返回，`sync_wait` 可继续退出并销毁栈上的 `state`，通知方随后访问已销毁的 `state.cv`。

这不是压力测试概率问题，而是静态生命周期合同风险。`mini_ref` 的同类实现已采用安全形态：`C09_Coroutines/exercises/Capstone5_mini_corolib/reference/include/mini_ref/mini.hpp:185-191` 在同一锁作用域内设置 `done` 并 `notify_one()`，等待方只有在 notify 调用结束并释放锁后才能取得锁、返回并销毁 state。

修复建议：把 `coroutine_study::detail::notify_sync_wait()` 的 `state.cv.notify_one()` 移入持锁作用域，或改成有明确 keepalive/ack 的完成状态。优先采用 mini_ref 已有形态，保持教学对照一致。补一个静态/代码审查检查，禁止栈状态 completion callback 采用“解锁后访问 cv”的形态；这个窗口很难用普通压力测试稳定证明。

### [HIGH] `mini_ref::shared_task` 会 resume 已销毁 waiter frame

File: `C09_Coroutines/exercises/Capstone5_mini_corolib/reference/include/mini_ref/mini.hpp:321`

问题：`shared_task::awaiter::await_suspend()` 把调用方裸 `coroutine_handle` 存入 `state->waiters`，没有注销或有效性 token。若等待 `shared_task` 的 coroutine 在 producer 完成前被销毁，`run()` 完成时在 `:343-349` 取出旧 handle 并 resume，触发 use-after-free。

已用真实源码和 ASan 复现，证据：

- 源码：`runtime-review-evidence/shared-task-early-destroy-asan.cpp`
- 命令：`runtime-review-evidence/shared-task-early-destroy-asan-command.txt`
- 元数据：`runtime-review-evidence/shared-task-early-destroy-asan-meta.json`
- 输出：`runtime-review-evidence/shared-task-early-destroy-asan-stderr.txt`
- exit_code: 1
- 关键输出：`ERROR: AddressSanitizer: heap-use-after-free`，栈顶指向 `mini_ref/mini.hpp:349` 的 `h.resume()`；释放栈指向 `mini_ref/mini.hpp:143` 的 `task<void>::~task()`。

修复建议：不要保存裸 waiter handle 作为唯一凭据。为每个等待者建立可注销的 waiter node/token；awaiter 析构时在未完成状态下注销或标记 inactive；完成线程只 resume 仍有效的 waiter。并补 ASan 回归：启动 waiter、让 source 挂起、销毁 waiter、再完成 source，要求不 resume stale handle、不 UAF。

### [HIGH] `mini_ref::when_any` 首个错误不请求 stop，无法收束等待 stop_token 的另一个分支

File: `C09_Coroutines/exercises/Capstone5_mini_corolib/reference/include/mini_ref/mini.hpp:492`

问题：`when_any_op::run_left()` / `run_right()` 成功路径在 `:491` / `:508` 对 winner 调用 `stop_.request_stop()`，但 catch 路径 `:492-495` / `:509-512` 只记录 `error_`，不请求 stop。`complete()` 在 `:472-476` 等两条 runner 都 final 后才恢复 parent。若一条分支失败，另一条分支正等待同一个 `stop_token` 才能完成，parent 永远等不到第二条 runner，`sync_wait(when_any(...))` 挂起。

已用真实源码有界复现，证据：

- 源码：`runtime-review-evidence/when_any_error_cancel_probe.cpp`
- 命令：`runtime-review-evidence/when-any-error-cancel-current-command.txt`
- 元数据：`runtime-review-evidence/when-any-error-cancel-current-meta.json`
- 输出：`runtime-review-evidence/when-any-error-cancel-current-stdout.txt` / `stderr.txt`
- 当前 `mini_ref/mini.hpp` SHA256：f6e35941658c1e1e5702b75b3721b8e2a6dbc94ba824a9d74168ff52edff3da4
- exit_code: 124，`timeout 5s` 命中。

修复建议：catch 路径应像成功路径一样在锁外请求 stop：持锁记录第一个 error 并设置局部 `should_stop=true`，释放锁后 `if (should_stop) stop_.request_stop();`。不要在锁内 request_stop，因为 stop callback 可能同步恢复另一分支。补回归：一条 task 立即抛异常，另一条 task 等 `stop_token` 回调恢复；修复后应抛出原错误并在外部 timeout 内退出。

## 已核对但不作为阻断

- `mini_ref::sync_wait_state::complete()` 当前把 `notify_one()` 放在持锁区间，见 `mini.hpp:185-191`；相对 `lazy_task.hpp` 的实现是安全模型。
- `mini_ref::task_scope::spawn()` 当前在 `threads_.emplace_back` 抛异常时有 rollback，见 `mini.hpp:579-605`。独立复验 `scope_spawn_failure_test.cpp` 通过，输出 `caught=1 in_flight=0 starts=0; waiting for drain` 与 `rejected work rolled back; accepted work drained`。证据在 `runtime-review-evidence/scope-spawn-failure-*`。
- `mini_ref::when_all` 的一次性 parent resume 逻辑用 `remaining_`、`waiting_` 和互斥保护，见 `mini.hpp:369-381` 与 `:390-395`；本轮未发现可归因阻断。结果移动抛出时，owner 仍在父 coroutine frame / `sync_wait(task<T>)` 参数生命周期内，不复现 S1 的 awaiter 丢 handle 问题。
- `stdexec_awaitable` 的同步完成和 early destruction 使用 `phase_` 状态机，见 `stdexec_awaitable.hpp:41-44`、`:89-109`；已有 reference tests 覆盖同步完成、异步完成、abandoned、error、stopped。当前审查未发现新阻断。
- 公共消费者索引已保存到 `runtime-review-evidence/consumer-callgraph.txt`。主要消费者包括 A2/A3/B2/B3/C1/C2/C3/G3/Capstone1/runtime_tests，以及 Capstone5 reference tests。C3 starter 中的 `detach(... catch (...) {})` 是教学 bad/占位语境，不按公共 Reference runtime 通过标准审批。

## 验证记录

保存的审查证据：

- `runtime-review-evidence/current-key-lines.txt`：当前关键行快照。
- `runtime-review-evidence/consumer-callgraph.txt`：公共 runtime 消费者索引。
- `runtime-review-evidence/when_any_error_cancel_probe.cpp`：`when_any` 错误取消收束复现。
- `runtime-review-evidence/when-any-error-cancel-current-*`：当前 hash 下 timeout 复现，exit_code 124。
- `runtime-review-evidence/shared-task-early-destroy-asan.cpp`：`shared_task` waiter 提前销毁 ASan 复现。
- `runtime-review-evidence/shared-task-early-destroy-asan-*`：ASan UAF 输出，exit_code 1。
- `runtime-review-evidence/scope-spawn-failure-*`：`task_scope` spawn rollback 通过证据，exit_code 0。

未运行 lsp_diagnostics / ast_grep_search：当前工具发现只返回 node_repl、Plugin Management、Sites，没有可用 lsp/ast-grep 工具。本轮用定向源码审查、消费者索引、WSL g++ C++20、`timeout` 和 ASan 作为替代验证。

## Recommendation

REQUEST CHANGES。

公共 runtime 在继续批量推广前，至少需要修复上面 3 个 HIGH：

1. `coroutine_study::notify_sync_wait` 的栈状态通知生命周期窗口。
2. `mini_ref::shared_task` 的 waiter 早销毁 UAF。
3. `mini_ref::when_any` 的错误路径 stop 收束缺失。

修复后请复跑对应最小复现、现有 mini_ref/runtime targeted tests，并由非作者复验。

---

## 复核更新：`when_any` 第 3 项语义重判

更新时间：2026-09-11。

主代理指出本课程 `when_any` 的明确契约不是首个 error fail-fast，而是：首个成功 value 胜出；只有成功 winner 才 `request_stop()`；无成功时等全部分支失败后传播首个异常。按该契约复核后，原报告中“`mini_ref::when_any` 首个错误不请求 stop”为 HIGH 阻断的判断撤回。

文档证据：

- `C09_Coroutines/exercises/Capstone5_mini_corolib/README.md:61-64`：首个成功 value 写入 winner；winner 后请求 stop；loser 仍完成收束；两个分支都失败时传播首个异常。
- `C09_Coroutines/09-模块G-symmetric_transfer与高级task.md:160-178`：真实契约是首个成功 value 获胜；没有任何成功 value 时，所有分支完成后传播首个异常；并提醒不要描述成“第一个完成立即返回”。
- `C09_Coroutines/14-第三阶段结课-mini协程库实现.md:188-200`：第一个成功 value 产生 winner 并 request_stop；失败分支只在没有 winner 时保存首个 exception；两个 runner 都完成后，有 winner 返回 winner，无 winner 抛首个异常。
- `C09_Coroutines/14-第三阶段结课-mini协程库实现.md:303-305`：stop_token 是协作取消信号，`when_any` 选出成功 winner 后帮助 loser 尽快退出。

新增有限反例验证保持 first-success 必要：

- 源码：`runtime-review-evidence/when-any-error-first-later-success.cpp`
- 命令：`runtime-review-evidence/when-any-error-first-later-success-command.txt`
- 元数据：`runtime-review-evidence/when-any-error-first-later-success-meta.json`
- stdout：`value=9 stop_requested=1`
- exit_code: 0

该反例中左分支先失败，右分支稍后成功。当前实现返回成功值 9，符合 first-success 契约。如果按原建议“首个 error 即 request_stop”修改，可能提前取消本来会成功的 fallback，改变课程语义。因此原 `when_any_error_cancel_probe.cpp` 的 timeout 只证明：若某个 loser 仅等待 stop 且没有自行完成/外部取消，那么当前语义需要活性前提；它不能单独证明算法死锁或实现缺陷。

修正后的建议：

- 不要求 `when_any` 在首个 error 时自动 `request_stop()`。
- 文档与测试应明确活性前提：在没有成功 winner 的路径上，所有失败/未胜出分支也必须最终完成，或由外部超时/取消机制完成；否则 fail-delay/first-success 语义本身会等待。
- 可保留 `when_any_error_cancel_probe.cpp` 作为“违反有限完成前提”的负例或教学说明，不作为阻断修复测试。

修正后的 Recommendation：仍为 REQUEST CHANGES，但阻断项只剩两个 HIGH：

1. `coroutine_study::notify_sync_wait` 的栈状态通知生命周期窗口。
2. `mini_ref::shared_task` 的 waiter 早销毁 UAF。

`mini_ref::when_any` 第 3 项降级为 LOW/文档与测试口径：补充“error-first later success 必须保留成功胜出”和“无成功路径要求所有分支有限完成或外部取消”的说明与代表测试即可。

## `notify_sync_wait` 安全模型给实现者

不能伪造实际崩溃；当前证据是静态生命周期合同风险。风险来自 `lazy_task.hpp:42-48`：完成方释放 mutex 后仍访问栈上 `state.cv`。由于 `sync_wait` 等待方可因 spurious wake 在 `done=true` 后返回并销毁栈上 state，完成方解锁后的 `notify_one()` 没有对象寿命保证。

推荐修复采用 `mini_ref` 已有模型：在持有 `state.mutex` 的同一临界区里设置 `done=true` 并调用 `state.cv.notify_one()`。这样等待方必须等通知方释放锁后才能从 `cv.wait(lock, pred)` 返回并离开 `sync_wait`，栈上 state 的生命周期覆盖整个 notify 调用。该修复不改变结果语义，只收紧完成通知的对象寿命边界。

---

## 复验更新：公共 runtime 两个 HIGH 关闭

更新时间：2026-09-11。
复验角色：native code-reviewer，非实现作者。
工作区 HEAD：75028ff2d312a4f1047e7bafd6cf4101f771a2ec。
当前 `lazy_task.hpp` SHA256：afa343676544a939d24c2152c1f96a1af8f3e6c401d2a7315cadd2cc20dbbaea。
当前 `mini_ref/mini.hpp` SHA256：4fcb231be03f6a88353a7a0cfdab8a85a1fc087720e5405f9dfe445bd12f6bb2。

结论：APPROVE runtime fixes。原报告保留 FAIL 记录与 `when_any` 语义撤回说明；本节记录修复后的独立复验。

### 1. `notify_sync_wait` 生命周期窗口已关闭

当前 `C09_Coroutines/exercises/include/coroutine_study/lazy_task.hpp:42-48` 已改为持锁设置 `done=true` 并在同一临界区调用 `state.cv.notify_one()`：

```cpp
auto& state = *static_cast<sync_wait_state*>(p);
std::lock_guard lock(state.mutex);
state.done = true;
state.cv.notify_one();
```

这与 `mini_ref::sync_wait_state::complete()` 的模型一致。这里仍按静态生命周期合同说明，不声称复现过实际 condition_variable UAF：修复点是让等待线程必须等通知方释放 mutex 后才能从 `cv.wait(lock, pred)` 返回并销毁栈上 state，从而覆盖通知方访问 `cv` 的对象寿命。

主代理 Windows targeted 证据回读：

- `C09_Coroutines/references/validation/c09-refresh/runtime-fixes/runtime-fixes-release-20260911T025656221510Z.json`
- command: `ctest --test-dir C09_Coroutines/exercises/build/c09-plan-baseline -C Release -R "mini_reference_shared_task|mini_reference_scope_spawn_failure|mini_reference_task_scope|runtime_await_resume_exception_test|runtime_sync_wait_reference_test" --output-on-failure`
- result: `100% tests passed, 0 tests failed out of 6`

### 2. `mini_ref::shared_task` waiter UAF 已关闭

当前 `mini_ref::shared_task` 修复形态：

- `mini.hpp:292-296` 定义 `registration`，包含 `registering/suspended/completed/cancelled` phase 和 caller handle。
- `mini.hpp:302` waiters 改为 `std::vector<std::weak_ptr<registration>>`。
- `mini.hpp:328-350` 的 `await_suspend` 用 local `shared_ptr` 保活 state 和 registration；source 同步完成时 `registering -> completed` 后 CAS 失败，返回 `false`，不会把 caller 当作已挂起再 resume。
- `mini.hpp:322` awaiter 析构把 registration 标为 `cancelled`。
- `mini.hpp:376-390` completion 逐个 lock weak registration；只有 `suspended -> completed` 才 `resume()`，避免已销毁 waiter 和前一个 callback 销毁后续 waiter 的 stale handle。
- `mini.hpp:356-374` producer 是私有 self-completing coroutine，`initial_suspend/final_suspend = suspend_never`，按值持有 `shared_ptr state`；state 不再 owns runner，避免 state/runner 环。

独立重跑旧 ASan repro：

- 源码：`runtime-review-evidence/shared-task-early-destroy-asan.cpp`
- 命令：`runtime-review-evidence/old-shared-task-early-destroy-asan-command.txt`
- 元数据：`runtime-review-evidence/old-shared-task-early-destroy-asan-meta.json`
- stdout：`out=0`
- stderr：空
- exit_code: 0

独立重跑新增 abandon test：

- 源码：`C09_Coroutines/exercises/Capstone5_mini_corolib/reference/tests/shared_task_abandon_test.cpp`
- 命令：`runtime-review-evidence/new-shared-task-abandon-asan-command.txt`
- 元数据：`runtime-review-evidence/new-shared-task-abandon-asan-meta.json`
- stdout：`shared_task: abandon, sibling destruction, cached read, owner-free completion checked`
- stderr：空
- exit_code: 0

主代理 WSL ASan/LSan 证据回读：

- `C09_Coroutines/references/validation/c09-refresh/runtime-fixes/shared-abandon-asan-run-20260911T025656029613Z.json`
- `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`
- stdout：`shared_task: abandon, sibling destruction, cached read, owner-free completion checked`
- exit_code: 0

新增测试覆盖有效：

- waiter 提前销毁后 producer 完成，不再 resume stale waiter。
- 第一个 callback/resumed waiter 销毁后续 waiter 后，completion 不恢复已销毁后续 waiter。
- 外部 `shared_task` owner 全释放时，producer 仍通过 self-completing runner 按值持有 state 到 source 收束；source 完成后 state 可释放。
- producer 完成后 late cached read 仍能拿到缓存值。

### 3. 必要交错探针通过

新增独立探针：`runtime-review-evidence/shared-task-interleavings.cpp`。

命令与输出：

- `runtime-review-evidence/shared-task-interleavings-command.txt`
- `runtime-review-evidence/shared-task-interleavings-meta.json`
- stdout：`shared_task interleavings: sync value, sync error, cross-thread value checked`
- stderr：空
- exit_code: 0

覆盖：

- source 同步完成：验证 `registering -> completed` 后 `await_suspend()` 返回 false，调用方继续并只消费一次。
- source 同步异常：验证 error 发布到 awaiter，调用方 catch 一次。
- source 跨线程完成：验证已挂起 waiter 的 `suspended -> completed` 只 resume 一次。

### 4. 边界声明

当前修复不宣称任意线程安全销毁。`coroutine_handle` 的基本前提仍是：同一 task owner 的销毁不能与另一个线程对同一 coroutine frame 的恢复并发发生；调用方要序列化该生命周期。已验证的是 shared_task completion 不再恢复已经按序销毁/注销的 waiter，也不因 source 同步完成窗口错挂起。

### 5. 修正后 Recommendation

APPROVE runtime fixes。

公共 runtime 先前两个 HIGH 已关闭：

1. `coroutine_study::notify_sync_wait` 的栈状态通知生命周期窗口。
2. `mini_ref::shared_task` 的 waiter 早销毁 UAF。

`mini_ref::when_any` 保持上一节复核结论：不是 HIGH 缺陷；按 LOW/文档与测试口径处理，即说明 first-success 契约和无成功路径的有限完成/外部取消前提。

---

## 复验更新：diagnostics-extra 两个 ASan 真实失败关闭

更新时间：2026-09-11。
复验角色：native code-reviewer，非实现作者。
工作区 HEAD：75028ff2d312a4f1047e7bafd6cf4101f771a2ec。
当前 `as_awaitable_test.cpp` SHA256：71212024ab04855663211cfe8603b1e4755512d1a7034fe57a26f10f0ae46267。
当前 `run_loop_test.cpp` SHA256：7d82761c11ed6e1f0f45c54b46b6337220b98b39203287ce95f9a2576b9fc898。
当前 `mini_ref/mini.hpp` SHA256：4fcb231be03f6a88353a7a0cfdab8a85a1fc087720e5405f9dfe445bd12f6bb2。
当前 `stdexec_awaitable.hpp` SHA256：1778db1bf86c8ec297d6549738a37eac62c90194295ddb1b166930142d591a41。

结论：APPROVE diagnostics-extra fixes。两个原先被 Windows 普通 CTest 漏掉、但由 GCC ASan 确证的 `stack-use-after-scope` 已由“立即调用捕获 coroutine lambda”改为命名 coroutine 函数，参数寿命现在由调用栈和 coroutine frame 明确承载；我独立复跑两个受影响目标，ASan/UBSan/LSan 均 `exit_code=0`。

### 1. 原始失败证据已回读

原始 stdexec bridge abandoned 路径失败：

- `C09_Coroutines/references/validation/c09-refresh/final/diagnostics-extra/stdexec-bridge-asan-run-20260911T041231937060Z.json`
- `exit_code: 1`
- 诊断：`AddressSanitizer: stack-use-after-scope`
- 栈顶源码：旧 `C09_Coroutines/exercises/Capstone5_mini_corolib/reference/tests/as_awaitable_test.cpp:116`
- 归因：立即调用的捕获 coroutine lambda 在 lambda closure 生命周期结束后才执行 coroutine body，捕获对象悬空。

原始 run_loop 路径失败：

- `C09_Coroutines/references/validation/c09-refresh/final/diagnostics-extra/run-loop-asan-before-run-20260911T041424188222Z.json`
- `exit_code: 1`
- 诊断：`AddressSanitizer: stack-use-after-scope`
- 栈顶源码：旧 `C09_Coroutines/exercises/Capstone5_mini_corolib/reference/tests/run_loop_test.cpp:15`
- 归因：立即调用的 `[&]` coroutine lambda 持有已经离开作用域的 closure 引用。

主代理修后证据也已回读：

- `stdexec-bridge-asan-after-run-20260911T042952498108Z.json`：`exit_code: 0`，stdout/stderr 空。
- `run-loop-asan-after-run-20260911T042929577082Z.json`：`exit_code: 0`，stdout/stderr 空。
- `rpc-asan-run-20260911T042952750652Z.json`：`status: PASS`，`exit_code: 0`，stdout/stderr 空。

### 2. 当前源码审查

`C09_Coroutines/exercises/Capstone5_mini_corolib/reference/tests/as_awaitable_test.cpp` 当前关键点：

- `:39-44` 的 `async_control` 字段已使用 `worker_started` 和 `body_resumes`，语义与检查一致。
- `:67-73` worker lambda 捕获 `control/value/receiver`，并通过 `worker_started`/`release_completion` 建立 abandoned 交错。
- `:89-94` 改为命名 coroutine `receive_async(std::shared_ptr<async_control> control, worker_group& workers, int value)`；`control` 按值进入 coroutine frame，`workers` 引用由测试作用域和 `workers.join()` 保证覆盖。
- `:100-114` 正常路径检查 result、completed、`body_resumes == 1`。
- `:116-127` abandoned 路径先启动并等 worker 真正进入，再销毁 task owner，随后释放 completion；检查 `completed == true` 且 `body_resumes == 0`，能证明 abandoned 后不恢复 coroutine body。

`C09_Coroutines/exercises/Capstone5_mini_corolib/reference/tests/run_loop_test.cpp` 当前关键点：

- `:10-13` 改为命名 coroutine `finish_scheduled(mini_ref::run_loop& loop, int& value, bool& done)`。
- `:19-24` 调用方显式持有 `auto t = finish_scheduled(...)`，`loop/value/done` 均活到 `t.start()` 与 `loop.run_one()` 之后；原立即调用 `[&]` coroutine lambda 的 closure 生命周期漏洞已消失。

同目录 coroutine lambda 扫描记录：`runtime-review-evidence/coroutine-lambda-scan.txt`。结果只剩：

- `as_awaitable_test.cpp:131` 和 `:143`：captureless coroutine lambda；局部 task 在 `sync_wait` 返回前同步消费，未携带 closure 捕获状态。
- `shared_task_test.cpp:23-24`：命名 local closure 捕获 `shared` by value，`first()/second()` 被同一作用域内的 `sync_wait(when_all(...))` 同步消费；closure 生命周期覆盖消费过程，和本次“立即调用捕获 coroutine lambda”失败模式不同。

### 3. 独立复跑证据

独立复跑使用 WSL g++，不下载依赖，复用现有 stdexec 缓存 include。两个目标均使用：`-std=c++20 -pthread -fsanitize=address,undefined -fno-omit-frame-pointer -g -Wall -Wextra -pedantic`，运行时使用 `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` 与 `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`。

run_loop after-run：

- command：`runtime-review-evidence/review-run-loop-asan-after-command.txt`
- meta：`runtime-review-evidence/review-run-loop-asan-after-meta.json`
- stdout：`runtime-review-evidence/review-run-loop-asan-after-stdout.txt`，空
- stderr：`runtime-review-evidence/review-run-loop-asan-after-stderr.txt`，空
- result：`exit_code=0`

stdexec bridge after-run：

- command：`runtime-review-evidence/review-stdexec-bridge-asan-after-command.txt`
- meta：`runtime-review-evidence/review-stdexec-bridge-asan-after-meta.json`
- stdout：`runtime-review-evidence/review-stdexec-bridge-asan-after-stdout.txt`，空
- stderr：`runtime-review-evidence/review-stdexec-bridge-asan-after-stderr.txt`，仅第三方/兼容性编译告警；无 sanitizer 运行期错误
- result：`exit_code=0`

sanitizer 文本扫描：`runtime-review-evidence/diagnostics-extra-sanitizer-scan.txt`，结果为 `NO sanitizer failure diagnostics in independent after-run stdout/stderr/meta`。

源码摘录与扫描证据：

- `runtime-review-evidence/diagnostics-extra-key-lines.txt`
- `runtime-review-evidence/coroutine-lambda-scan.txt`

### 4. 边界与未扩展事项

本轮只独立复跑这两个新增 diagnostics-extra ASan 修复目标，并回读 RPC Reference ASan PASS 文件；没有重新宣称全量约 16 个 ASan 主体由我独立执行。TSan 仍按环境 capability probe 失败处理为 SKIP；本轮未把 TSan SKIP 当作主体 PASS。

`stdexec` bridge after-run 编译阶段有上游 `stdexec` 头内的 deprecated/extra-semi 告警，但退出码为 0，且运行期没有 ASan/UBSan/LSan 诊断；不构成本轮阻断。

### 5. Recommendation

APPROVE diagnostics-extra fixes。

本轮未发现 CRITICAL/HIGH/MEDIUM 阻断。两个真实 ASan `stack-use-after-scope` 缺陷均有原始失败证据、源码归因、当前源码修复点和独立 after-run 证据闭环。

---

## 有界同根检查：G1_shared_task main/solution

更新时间：2026-09-11。
复验角色：native code-reviewer，非实现作者。
工作区 HEAD：75028ff2d312a4f1047e7bafd6cf4101f771a2ec。
当前 `G1_shared_task/main.cpp` SHA256：720f1308a37834f99da9abda0a83d8276bf7700afc403196feac45d288fa5fd9。
当前 `G1_shared_task/solution.cpp` SHA256：d73e249c7097445a13102cf8ebaf934500b0627ed8e1a53a427ca17b89b1e7e2。
当前 `G1_shared_task/README.md` SHA256：dc0d3dc350e826ee8722ec5a0315f0acc6d9962a0eadc31c0c264bfa10c95bd7。

结论：COMMENT / 非阻断，前提是 G1 保持“有限教学基线”口径。G1 没有复制 mini_ref 先前的 `shared_state -> runner -> state` 强引用环；但 `main.cpp` 与 `solution.cpp` 都有裸 waiter handle/list，同根生命周期风险可以用 ASan 最小复现确证。由于 `README.md:60-62` 已明确把提前销毁 waiter、登记和完成并发、最后 owner 释放时 producer 关系列为基础实现未处理边界，本轮不把“提前销毁 waiter 后完成”按 HIGH 阻断；若后续正文或 checker 声称 G1 是完整/安全 `shared_task`，必须修或降级表述。

### 1. 静态核对

`solution.cpp`：

- `:35` promise 只持有 `std::weak_ptr<control_block> owner`。
- `:76-82` `control_block` 持有 producer coroutine handle、`started/completed` 和 `std::vector<std::coroutine_handle<>> waiters`。
- `:78` `control_block::~control_block()` 销毁 producer frame。
- `:40-46` final awaiter 将 `completed=true`，move 出 waiters，逐个 `resume()` 或返回第一个 waiter handle。
- `:63-65` awaiter 将 caller handle 裸存入 `cb->waiters`，没有 registration/注销机制。

因此 `solution.cpp` 没有 state/runner 环；合法 awaiter 持有 `shared_ptr<control_block>` 时，producer frame 的寿命由 control_block 保证。但如果已登记 waiter coroutine owner 在 producer 完成前被销毁，`cb->waiters` 内保存的裸 handle 会悬空。

`main.cpp`：

- `:166-179` 手写 `control_block` 用 ref_count 控制 producer frame，无 runner 环。
- `:117-118` promise 保存 intrusive `awaiter* waiters_head`。
- `:145-155` final awaiter 读取 intrusive node 并 resume/返回 caller。
- `:217-233` awaiter 自身就是 intrusive list node，存放在 waiter coroutine frame 中；析构时没有从 `waiters_head` 注销。

因此 `main.cpp` 同样没有 state/runner 环，但复制了裸 waiter/list 生命周期风险。该 starter 还依赖外部显式 `st.resume()` 启动 producer；这和 `solution.cpp` 的首个 awaiter 自动 start 模型不同，本轮只作为 G1 starter 边界记录，不展开为全课整改项。

### 2. 最小 ASan 复现

`solution.cpp` 的提前销毁 waiter 复现：

- source：`runtime-review-evidence/g1-shared-task-abandoned-waiter-asan.cpp`
- command：`runtime-review-evidence/g1-shared-task-abandoned-waiter-asan-command.txt`
- meta：`runtime-review-evidence/g1-shared-task-abandoned-waiter-asan-meta.json`
- output：`runtime-review-evidence/g1-shared-task-abandoned-waiter-asan-output.txt`
- result：`exit_code=1`
- 诊断：`AddressSanitizer: heap-use-after-free`
- 归因：waiter coroutine frame 在 `void_task::~void_task()` 中被销毁，producer 完成后 final suspend 返回/恢复保存的旧 waiter handle。

`main.cpp` 的提前销毁 waiter 复现：

- source：`runtime-review-evidence/g1-main-shared-task-abandoned-waiter-asan.cpp`
- command：`runtime-review-evidence/g1-main-shared-task-abandoned-waiter-asan-command.txt`
- meta：`runtime-review-evidence/g1-main-shared-task-abandoned-waiter-asan-meta.json`
- output：`runtime-review-evidence/g1-main-shared-task-abandoned-waiter-asan-output.txt`
- result：`exit_code=1`
- 诊断：`AddressSanitizer: heap-use-after-free`
- 栈顶归因行：`G1_shared_task/main.cpp:150`，final awaiter 从已经释放的 awaiter node 读取 `node->next`。

这两个复现是已确证同根风险，但触发条件是“已登记 waiter owner 提前销毁后 producer 继续完成”。按本轮任务边界，它属于 README 已声明未支持的提前销毁边界，不等同于 mini_ref 公共 Reference runtime 的未声明 HIGH。

### 3. 合法 owner 释放路径核对

`solution.cpp` 合法路径：

- source：`runtime-review-evidence/g1-shared-task-legal-owner-release-asan.cpp`
- command：`runtime-review-evidence/g1-shared-task-legal-owner-release-asan-command.txt`
- meta：`runtime-review-evidence/g1-shared-task-legal-owner-release-asan-meta.json`
- output：`runtime-review-evidence/g1-shared-task-legal-owner-release-asan-output.txt`
- result：`exit_code=0`
- 覆盖：waiter 活着时 producer 外部完成，然后全部 owner 释放；以及临时 `shared_task` 被 `co_await` 后同步完成并释放。

`main.cpp` 合法路径：

- source：`runtime-review-evidence/g1-main-shared-task-legal-owner-release-asan.cpp`
- command：`runtime-review-evidence/g1-main-shared-task-legal-owner-release-asan-command.txt`
- meta：`runtime-review-evidence/g1-main-shared-task-legal-owner-release-asan-meta.json`
- output：`runtime-review-evidence/g1-main-shared-task-legal-owner-release-asan-output.txt`
- result：`exit_code=0`
- 覆盖：按 starter 自身模型先显式 `st.resume()` 启动 producer，waiter 活着时 producer 完成，然后全部 owner 释放。

这说明本轮没有发现 G1 中“运行中的 runner 因 state 析构被销毁”的同根环问题；G1 的主要生命周期风险是裸 waiter 注册缺少注销。

### 4. 修法/取舍建议

若 G1 要作为完整安全版本讲解：复用 Capstone5 `mini_ref::shared_task` 已修模型，或把 G1 的 waiters 改为 registration 对象加取消/完成 phase，completion 只恢复仍存活且 `suspended -> completed` 成功的 registration；同时补 abandoned waiter 和 sibling-destroy 回归。

若 G1 保留为有限教学基线：保留当前代码可以接受，但正文和 checker 不应声称支持提前销毁 waiter、任意跨线程 destroy 或完整公共 runtime 级安全；`README.md:60-62` 的边界声明必须保留，并建议在 G1 正文/注释中更显式地写出“裸 waiter handle 仅用于单线程、waiter owner 活到 producer 完成”的前提。

### 5. Recommendation

COMMENT。当前 G1 检查不阻断公共 runtime 修复通过；但已留下可复现证据，证明 G1 的 main/solution 共享裸 waiter 生命周期风险。最终交付时必须二选一：明确作为有限教学基线保留，或按 Capstone5 修复模型升级，不能同时声称完整 shared_task 安全语义。
