# C3：可中断等待与数据优先策略

先读 [正文](../../chapters/06-cancellation-and-shutdown.md)，再对照 [完整 Reference](solution.cpp)。本题默认 C++23，所有检查使用 Release 下仍有效的 cs::check；Starter 默认在启动并发前以 1 退出，学生实现的等待由外部 CTest 超时限制。

## 必做 Parts

1. 用 condition_variable_any::wait(lock,token,predicate) 实现 mailbox::take，正常投递 42 并用 future 确认已取到。
2. 空信箱等待时 request_stop，无需应用手写 notify；检查取消前置和取消并发两种情况。
3. 在已停止 token 下投递 7，检查数据优先；占用槽位上的 put(8) 必须失败，禁止覆盖旧消息。
4. 定时可中断等待检查“没有 stop 也可以 false”；复用共享 bounded_channel 验证 close 后仍按 1、2 排空。

## Reference 对照与运行观察

mailbox 是完整单槽、单消费者实现；main 按正常值、取消、已停止、数据/取消同时可见、期限和通道关闭逐项检查。异步异常经 future.get 传播。

## 学生入口与检查

[main.cpp](main.cpp) 提供独立 Starter，按中文 TODO 完成对应学生函数或方法；不要直接包含 solution.cpp。完成一个 Part 后把该项 `part*_done` 改为 true。标记仅解除启动保护，不代替验收：只有实际调用学生实现的检查通过才返回 0。未完成时输出 `STARTER INCOMPLETE` 并返回 1，发生在任何线程创建之前。

Part 1/2/3 实现 `student_mailbox::put/take`：拒绝覆盖、可中断等待、数据优先。Part 4 实现 `student_expired_wait`。所有信箱检查调用学生类型；最后使用公共通道的 close/drain 是已经完成的比较示例，不替代信箱检查。

改动 main.cpp 后必须运行下面的 student 命令。Reference 的通过不说明学生实现正确。若只改标记、没有补全函数，TODO 或检查仍会失败；错误同步协议也可能被 CTest 的 30 秒超时终止。

mailbox 不提供多消费者数据流结束协议；需要全局 close/drain 与阻塞背压时，使用 C2 的公共通道。

## 复盘答案

**wait 返回 true 是没取消吗？** 不是，只是最终谓词为真。数据与停止同时可见时，本题取走数据。

**false 一定是取消吗？** 无期限重载在 stop 且谓词 false 时返回 false；定时重载还可能因期限到达，不能统一解释成取消。

**condition_variable 能直接接 token 吗？** 不能，标准可中断重载在 condition_variable_any。自己只在 while 里检查 token 不能代替内部通知登记。

**close 与停止一个 take 有何差别？** close 拒绝全局新数据并保留旧值供排空；take 取消只放弃一次等待，不关闭生产方。

**为何不用两次 put 中间 sleep？** sleep 不能证明第一项已消费，可能发生覆盖。这里用 future 确认交付，put 也明确拒绝占用槽位。

## 构建与验证

以下两套命令都在 `C08_Concurrency/exercises` 工作目录运行；Windows 示例使用 Release，构建目录分开以避免缓存选项混淆。

Reference 基准答案门禁（不运行 main）：

```powershell
cmake -S C3_interruptible_wait -B build/reference-C3_interruptible_wait -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/reference-C3_interruptible_wait --config Release --target C3_interruptible_wait_reference
ctest --test-dir build/reference-C3_interruptible_wait -C Release -R "^C3_interruptible_wait_reference$" --no-tests=error --output-on-failure
```

学生实现验收（实际运行 main）：

```powershell
cmake -S C3_interruptible_wait -B build/student-C3_interruptible_wait -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-C3_interruptible_wait --config Release --target C3_interruptible_wait
ctest --test-dir build/student-C3_interruptible_wait -C Release -R "^C3_interruptible_wait_student$" --no-tests=error --output-on-failure
```

原样 Starter 返回 1，student CTest 报 Failed 是预期；不能把它标为 SKIP 或 WILL_FAIL 来制造通过。默认 `CONCURRENCY_STUDY_TEST_STARTERS=OFF`，未完成学生测试不进入 Reference 门禁。可选直接有界运行：

```powershell
python tools/run_diagnostic.py --timeout 5 -- ./build/student-C3_interruptible_wait/Release/C3_interruptible_wait.exe
```

成功只说明本次输入与实际交错通过相应检查；文字推导、一般交错、未测平台仍须单独核验。规范和完整答案见正文；不要求特定加速。
