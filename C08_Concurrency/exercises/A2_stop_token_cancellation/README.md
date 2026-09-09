# A2：共享停止状态、回调与寿命

先读 [正文](../../chapters/06-cancellation-and-shutdown.md)，再对照 [完整 Reference](solution.cpp)。本题默认 C++23，所有检查使用 Release 下仍有效的 cs::check；Starter 默认在启动并发前以 1 退出，学生实现的等待由外部 CTest 超时限制。

## 必做 Parts

1. 写 jthread 的 token 轮询循环，主线程显式 request_stop 并 join；检查确实观察停止，重复请求不重复改变状态。
2. 复制 stop_source，经副本请求并检查所有句柄看到相同状态；验证默认 token 和未停止但没有 source 的 orphan token。
3. 记录已注册 callback 的执行线程，检查 request_stop 返回前已执行；在已停止 token 上注册 late callback，验证构造时同步执行。
4. 回调已在另一线程开始执行后并发注销，验证注销完成时回调已结束；用独立外部 source 控制异步任务。
5. 若 CS_HAS_INPLACE_STOP_TOKEN 可用，运行 source/token/callback 的 C++26 附加检查，保持 source 寿命覆盖所有使用者。

## Reference 对照与运行观察

`polling()`、`sources_and_callbacks()`、`callback_destruction()` 以及 main 的能力宏分支各有真实可运行实现。普通 worker 异常由 exception_ptr 或 future 回传。

## 学生入口与检查

[main.cpp](main.cpp) 提供独立 Starter，按中文 TODO 完成对应学生函数或方法；不要直接包含 solution.cpp。完成一个 Part 后把该项 `part*_done` 改为 true。标记仅解除启动保护，不代替验收：只有实际调用学生实现的检查通过才返回 0。未完成时输出 `STARTER INCOMPLETE` 并返回 1，发生在任何线程创建之前。

Part 1 实现 `student_poll`；Part 2 实现 `student_copy_source`；Part 3 实现 `student_register`；Part 4 实现 `student_unregister` 并观察外部 source；Part 5 在能力宏开启时实现 `student_inplace`。检查请求观察、复制共享态、回调线程、late 注册与注销后完成状态；宏为 0 时不要求 Part 5 完成标记。

改动 main.cpp 后必须运行下面的 student 命令。Reference 的通过不说明学生实现正确。若只改标记、没有补全函数，TODO 或检查仍会失败；错误同步协议也可能被 CTest 的 30 秒超时终止。

本机默认能力宏为 0 时打印 inplace 专项 SKIP，退出成功仅代表基线检查成功，不代表实现或测试了 C++26 分支。

## 复盘答案

**request_stop 是强杀吗？** 不是，任务必须在检查点响应；阻塞 I/O 不会仅因 token 被修改而自动结束。

**复制 source 是新取消域吗？** 不是，复制句柄共享同一状态；想开始全新操作要新建 source。

**回调总在 worker 上执行吗？** 不是，注册时已停止就在注册线程同步执行；预先登记的回调由成功请求方同步调用，顺序不保证。

**回调析构会等待谁？** 若该回调在另一线程执行，会等它返回；不可持着它需要的锁去注销。回调异常逃逸会 terminate，所以检查在回调外完成。

**inplace token 延长 source 寿命吗？** 不会，它关联内嵌状态；必须让查询、回调、worker 全部先结束。缺能力时只跳过该专项，C++23 基线仍检查。

## 构建与验证

以下两套命令都在 `C08_Concurrency/exercises` 工作目录运行；Windows 示例使用 Release，构建目录分开以避免缓存选项混淆。

Reference 基准答案门禁（不运行 main）：

```powershell
cmake -S A2_stop_token_cancellation -B build/reference-A2_stop_token_cancellation -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/reference-A2_stop_token_cancellation --config Release --target A2_stop_token_cancellation_reference
ctest --test-dir build/reference-A2_stop_token_cancellation -C Release -R "^A2_stop_token_cancellation_reference$" --no-tests=error --output-on-failure
```

学生实现验收（实际运行 main）：

```powershell
cmake -S A2_stop_token_cancellation -B build/student-A2_stop_token_cancellation -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-A2_stop_token_cancellation --config Release --target A2_stop_token_cancellation
ctest --test-dir build/student-A2_stop_token_cancellation -C Release -R "^A2_stop_token_cancellation_student$" --no-tests=error --output-on-failure
```

原样 Starter 返回 1，student CTest 报 Failed 是预期；不能把它标为 SKIP 或 WILL_FAIL 来制造通过。默认 `CONCURRENCY_STUDY_TEST_STARTERS=OFF`，未完成学生测试不进入 Reference 门禁。可选直接有界运行：

```powershell
python tools/run_diagnostic.py --timeout 5 -- ./build/student-A2_stop_token_cancellation/Release/A2_stop_token_cancellation.exe
```

成功只说明本次输入与实际交错通过相应检查；文字推导、一般交错、未测平台仍须单独核验。规范和完整答案见正文；不要求特定加速。
