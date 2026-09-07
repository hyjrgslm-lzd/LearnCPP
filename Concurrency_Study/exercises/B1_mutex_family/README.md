# B1：保护共享不变量与锁所有权

先读 [正文](../../chapters/04-shared-state-and-locks.md)，再对照 [完整 Reference](solution.cpp)。本题默认 C++23，所有检查使用 Release 下仍有效的 cs::check；Starter 默认在启动并发前以 1 退出，学生实现的等待由外部 CTest 超时限制。

## 必做 Parts

1. 解释普通 counter 的冲突访问为何构成数据竞争；普通运行不得执行该反例。用四个异步任务各做 2000 次加锁自增，检查 8000。
2. 实现 config 的独占更新与共享快照，让 revision 与 twice_revision 始终满足倍数关系；检查每次快照及最终版本。
3. 展示 defer_lock、unique_lock 移动、提前 unlock 和异常展开。解锁后等待另一个写者完成，同时保留自己的值快照。
4. 运行 timed_mutex 限时失败与 recursive_mutex 嵌套获取，说明它们各自的前提。

## Reference 对照与运行观察

`counter_and_config()` 对应 Part 1、2；`ownership()` 对应 Part 3；`timed_and_recursive()` 对应 Part 4。worker 中的检查通过 async future.get 传播。

## 学生入口与检查

[main.cpp](main.cpp) 提供独立 Starter，按中文 TODO 完成对应学生函数或方法；不要直接包含 solution.cpp。完成一个 Part 后把该项 `part*_done` 改为 true。标记仅解除启动保护，不代替验收：只有实际调用学生实现的检查通过才返回 0。未完成时输出 `STARTER INCOMPLETE` 并返回 1，发生在任何线程创建之前。

Part 1 对应 `student_increment`；Part 2 对应 `student_config::set/snapshot`；Part 3 对应 `student_acquire_and_move` 与 `student_snapshot_and_unlock`；Part 4 对应 `student_timed_try/student_recursive`。`check_student()` 用 8000 次自增、配置关系、所有权状态和具体返回值检查学生实现；异常展开的局部示例已经给出。

改动 main.cpp 后必须运行下面的 student 命令。Reference 的通过不说明学生实现正确。若只改标记、没有补全函数，TODO 或检查仍会失败；错误同步协议也可能被 CTest 的 30 秒超时终止。

危险路径需要 `CONCURRENCY_STUDY_ENABLE_UNSAFE_DEMOS=ON` 且显式 `--unsafe-race`。未开启时返回 77；即便编译开启，不传参数仍只跑安全检查。

危险诊断参数只由 `B1_mutex_family_reference` 可执行文件处理，Starter 不运行这些路径。

## 复盘答案

**快照中的字段都合法，为什么仍可能错？** 新版本号和旧派生值可能分别合法却组合错误；必须检查它们的关系，并在同一次临界区读取。

**shared_lock 一定比独占读快吗？** 不保证。它允许并发读，不保证实际调度或特定加速，簿记与争用代价需要另测。

**unique_lock 移走后旧对象还会解锁吗？** 不会再拥有那份锁，目标对象承担释放义务。普通 mutex 的线程所有权规则仍然成立。

**定时失败为什么不靠 sleep？** 主线程拥有 mutex，直到异步 try_lock_for 结束才释放，因此那次尝试不可能获取成功。

**数据竞争如何诊断？** 单独以 unsafe 选项配置，再显式传 --unsafe-race。它是 UB，无合法的期望数字，不参与正确性或性能排名。

## 构建与验证

以下两套命令都在 `Concurrency_Study/exercises` 工作目录运行；Windows 示例使用 Release，构建目录分开以避免缓存选项混淆。

Reference 基准答案门禁（不运行 main）：

```powershell
cmake -S B1_mutex_family -B build/reference-B1_mutex_family -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/reference-B1_mutex_family --config Release --target B1_mutex_family_reference
ctest --test-dir build/reference-B1_mutex_family -C Release -R "^B1_mutex_family_reference$" --no-tests=error --output-on-failure
```

学生实现验收（实际运行 main）：

```powershell
cmake -S B1_mutex_family -B build/student-B1_mutex_family -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-B1_mutex_family --config Release --target B1_mutex_family
ctest --test-dir build/student-B1_mutex_family -C Release -R "^B1_mutex_family_student$" --no-tests=error --output-on-failure
```

原样 Starter 返回 1，student CTest 报 Failed 是预期；不能把它标为 SKIP 或 WILL_FAIL 来制造通过。默认 `CONCURRENCY_STUDY_TEST_STARTERS=OFF`，未完成学生测试不进入 Reference 门禁。可选直接有界运行：

```powershell
python tools/run_diagnostic.py --timeout 5 -- ./build/student-B1_mutex_family/Release/B1_mutex_family.exe
```

成功只说明本次输入与实际交错通过相应检查；文字推导、一般交错、未测平台仍须单独核验。规范和完整答案见正文；不要求特定加速。
