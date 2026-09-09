# B2：多锁避免算法与完整转账

先读 [正文](../../chapters/04-shared-state-and-locks.md)，再对照 [完整 Reference](solution.cpp)。本题默认 C++23，所有检查使用 Release 下仍有效的 cs::check；Starter 默认在启动并发前以 1 退出，学生实现的等待由外部 CTest 超时限制。

## 必做 Parts

1. 画出 A→B 与 B→A 的等待环，解释互斥、持有并等待、不可抢占、循环等待。默认不执行永久死锁反例。
2. 用 scoped_lock 完成一次检查与双账户更新；对自转账先处理别名，禁止同一 mutex 传入两次。
3. 再实现 std::lock + adopt_lock、std::less<const void*> 固定全序两个版本。每版并发执行双向转账，检查精确余额。

## Reference 对照与运行观察

`transfer(...,variant)` 的三个分支分别是独立实现，main 对每个分支运行同样输入；最终 A=11000、B=9000。`isolated_deadlock()` 是仅供外部超时诊断的反例。

## 学生入口与检查

[main.cpp](main.cpp) 提供独立 Starter，按中文 TODO 完成对应学生函数或方法；不要直接包含 solution.cpp。完成一个 Part 后把该项 `part*_done` 改为 true。标记仅解除启动保护，不代替验收：只有实际调用学生实现的检查通过才返回 0。未完成时输出 `STARTER INCOMPLETE` 并返回 1，发生在任何线程创建之前。

Part 1 填写 `student_wait_cycle` 与四条件注释；Part 2 实现 `student_scoped_transfer`；Part 3 实现 `student_adopt_transfer/student_ordered_transfer`。三版都用 1000 次双向转账检查 11000/9000、自转账及无效金额不改余额。等待图说明还需人工核对，非空检查不证明解释正确。

改动 main.cpp 后必须运行下面的 student 命令。Reference 的通过不说明学生实现正确。若只改标记、没有补全函数，TODO 或检查仍会失败；错误同步协议也可能被 CTest 的 30 秒超时终止。

危险路径需要 `CONCURRENCY_STUDY_ENABLE_UNSAFE_DEMOS=ON` 与 `--unsafe-deadlock`。反例用 barrier 确认两线程各持一把锁，再相互等待；必须用外部进程超时，普通 CTest 永不传该参数。

危险诊断参数只由 `B2_deadlock_scoped_lock_reference` 可执行文件处理，Starter 不运行这些路径。

## 复盘答案

**scoped_lock 是一次原子取多锁吗？** 不是。正常构造返回后拥有各锁，获取期间执行标准的死锁避免算法；业务更新对遵守同一锁协议的观察者不可拆开，才是这里的业务原子性。

**为什么不能用不相关指针的 < 排序？** 内建关系比较不能在这里充当可移植全序；std::less<const void*> 或唯一固定 ID 能建立一致顺序。

**自转账为何提前返回？** 否则同一非递归 mutex 被重复传给多锁算法，违反获取前提。

**加超时是否修复死锁？** 超时能让某次等待放弃，但若仍保留错误资源依赖，系统可能反复失败。统一顺序或多锁算法消除特定锁集合的获取等待环，外部 future/第三把锁仍需分析。

## 构建与验证

以下两套命令都在 `C08_Concurrency/exercises` 工作目录运行；Windows 示例使用 Release，构建目录分开以避免缓存选项混淆。

Reference 基准答案门禁（不运行 main）：

```powershell
cmake -S B2_deadlock_scoped_lock -B build/reference-B2_deadlock_scoped_lock -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/reference-B2_deadlock_scoped_lock --config Release --target B2_deadlock_scoped_lock_reference
ctest --test-dir build/reference-B2_deadlock_scoped_lock -C Release -R "^B2_deadlock_scoped_lock_reference$" --no-tests=error --output-on-failure
```

学生实现验收（实际运行 main）：

```powershell
cmake -S B2_deadlock_scoped_lock -B build/student-B2_deadlock_scoped_lock -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-B2_deadlock_scoped_lock --config Release --target B2_deadlock_scoped_lock
ctest --test-dir build/student-B2_deadlock_scoped_lock -C Release -R "^B2_deadlock_scoped_lock_student$" --no-tests=error --output-on-failure
```

原样 Starter 返回 1，student CTest 报 Failed 是预期；不能把它标为 SKIP 或 WILL_FAIL 来制造通过。默认 `CONCURRENCY_STUDY_TEST_STARTERS=OFF`，未完成学生测试不进入 Reference 门禁。可选直接有界运行：

```powershell
python tools/run_diagnostic.py --timeout 5 -- ./build/student-B2_deadlock_scoped_lock/Release/B2_deadlock_scoped_lock.exe
```

成功只说明本次输入与实际交错通过相应检查；文字推导、一般交错、未测平台仍须单独核验。规范和完整答案见正文；不要求特定加速。
