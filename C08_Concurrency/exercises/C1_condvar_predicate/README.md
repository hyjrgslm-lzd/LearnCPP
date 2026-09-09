# C1：谓词、无关通知与固定期限

先读 [正文](../../chapters/05-waiting-and-channels.md)，再对照 [完整 Reference](solution.cpp)。本题默认 C++23，所有检查使用 Release 下仍有效的 cs::check；Starter 默认在启动并发前以 1 退出，学生实现的等待由外部 CTest 超时限制。

## 必做 Parts

1. 实现 ready/payload 交接，检查通知早于 wait 时仍能读取 payload=42。
2. 启动三个等待者，持锁登记报到并等待；发送 ready=false 的通知，确认发生了多次谓词检查，再发布 99 并 notify_all。
3. 用同一个 steady_clock deadline 检查谓词 false 的期限失败，以及过期期限但谓词已真的成功。
4. 解释裸 wait 丢失早期通知的原因；可选用独立有期限诊断，不把虚假唤醒或 timeout 中某一种写成必然输出。

## Reference 对照与运行观察

solution.cpp 的三个安全代码段逐项检查早发布、广播与期限。predicate_checks 的条件变量握手用于确认重新检查，完全不依赖 sleep。

## 学生入口与检查

[main.cpp](main.cpp) 提供独立 Starter，按中文 TODO 完成对应学生函数或方法；不要直接包含 solution.cpp。完成一个 Part 后把该项 `part*_done` 改为 true。标记仅解除启动保护，不代替验收：只有实际调用学生实现的检查通过才返回 0。未完成时输出 `STARTER INCOMPLETE` 并返回 1，发生在任何线程创建之前。

Part 1 实现 `student_wait`；Part 2 实现 `student_broadcast`；Part 3 实现 `student_wait_until`；Part 4 填写遗漏通知说明。检查早发布的 42、false 条件下多次谓词检查、三名等待者收到 99，以及同一期限的 false/true 分支；说明文字仍需人工核对。

改动 main.cpp 后必须运行下面的 student 命令。Reference 的通过不说明学生实现正确。若只改标记、没有补全函数，TODO 或检查仍会失败；错误同步协议也可能被 CTest 的 30 秒超时终止。

`--unsafe-notify` 只在 unsafe 编译选项开启时执行；它有有限期限，但不验证某一种裸等待返回原因必然发生。普通运行不进入该诊断。

危险诊断参数只由 `C1_condvar_predicate_reference` 可执行文件处理，Starter 不运行这些路径。

## 复盘答案

**wait 解锁期间谁保护状态？** 其他访问者继续遵守同一 mutex；wait 返回前重新获得它，之后再使用谓词及 payload。

**无关 notify_all 为何不误放行？** 每次重新持锁都检查 ready，false 就继续等。通知不是业务令牌。

**为什么不循环 wait_for(100ms)？** 每次重新给足预算可能把总等待无限延长；计算一次 deadline，或用标准谓词版 wait_for。

**false 是否说明接下来没有数据？** 只说明返回时持锁观察到谓词为假；解锁后状态可以立即变化。

**notify_one 为什么不够广播？** 已在等待的其他线程没有全部重新检查的保证；虚假唤醒不能作为协议依赖。

## 构建与验证

以下两套命令都在 `C08_Concurrency/exercises` 工作目录运行；Windows 示例使用 Release，构建目录分开以避免缓存选项混淆。

Reference 基准答案门禁（不运行 main）：

```powershell
cmake -S C1_condvar_predicate -B build/reference-C1_condvar_predicate -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/reference-C1_condvar_predicate --config Release --target C1_condvar_predicate_reference
ctest --test-dir build/reference-C1_condvar_predicate -C Release -R "^C1_condvar_predicate_reference$" --no-tests=error --output-on-failure
```

学生实现验收（实际运行 main）：

```powershell
cmake -S C1_condvar_predicate -B build/student-C1_condvar_predicate -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-C1_condvar_predicate --config Release --target C1_condvar_predicate
ctest --test-dir build/student-C1_condvar_predicate -C Release -R "^C1_condvar_predicate_student$" --no-tests=error --output-on-failure
```

原样 Starter 返回 1，student CTest 报 Failed 是预期；不能把它标为 SKIP 或 WILL_FAIL 来制造通过。默认 `CONCURRENCY_STUDY_TEST_STARTERS=OFF`，未完成学生测试不进入 Reference 门禁。可选直接有界运行：

```powershell
python tools/run_diagnostic.py --timeout 5 -- ./build/student-C1_condvar_predicate/Release/C1_condvar_predicate.exe
```

成功只说明本次输入与实际交错通过相应检查；文字推导、一般交错、未测平台仍须单独核验。规范和完整答案见正文；不要求特定加速。
