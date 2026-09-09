# B3：一次成功初始化、重试与发布

先读 [正文](../../chapters/04-shared-state-and-locks.md)，再对照 [完整 Reference](solution.cpp)。本题默认 C++23，所有检查使用 Release 下仍有效的 cs::check；Starter 默认在启动并发前以 1 退出，学生实现的等待由外部 CTest 超时限制。

## 必做 Parts

1. 八个异步调用者经同一 once_flag 获得资源，检查都观察到 123。
2. 第一次 active 初始化故意抛异常，第二次成功；检查总尝试数为二、失败数为一，成功后不再进入初始化体。
3. 用函数内 static 对照，八个调用者都取得 456，构造计数恰好为一；解释两种所有权安排。
4. 使用带参 call_once 初始化，说明参数属于成功的那次调用，不能假定某个竞争调用者必定胜出。

## Reference 对照与运行观察

solution.cpp 的异步循环覆盖前三项，call_once 的参数 123 覆盖 Part 4；local_resource() 是独立的局部静态版本。

## 学生入口与检查

[main.cpp](main.cpp) 提供独立 Starter，按中文 TODO 完成对应学生函数或方法；不要直接包含 solution.cpp。完成一个 Part 后把该项 `part*_done` 改为 true。标记仅解除启动保护，不代替验收：只有实际调用学生实现的检查通过才返回 0。未完成时输出 `STARTER INCOMPLETE` 并返回 1，发生在任何线程创建之前。

Part 1 实现 `student_resource::get`；Part 2/4 实现 `initialize(initial)` 的失败重试与参数发布；Part 3 实现 `student_local_resource`。检查八个读取者、恰好两次尝试与一次失败、局部 static 恰好一次，以及成功后传 999 仍读取首次发布的 123。

改动 main.cpp 后必须运行下面的 student 命令。Reference 的通过不说明学生实现正确。若只改标记、没有补全函数，TODO 或检查仍会失败；错误同步协议也可能被 CTest 的 30 秒超时终止。

此题不运行 double-checked locking 的 UB 版本；在正文按冲突访问和同步边界分析错误，不要求通过错误结果演示来验证 UB。

## 复盘答案

**恰好一次指什么？** 指本实验一次成功返回。一般规范至多一次 returning，exceptional 调用可以多次，甚至没有成功。

**初始化写入为何可见？** returning 与所有 passive 返回同步；active 调用本身也有全序和相邻同步。调用者必须先经过 call_once。

**失败会回滚副作用吗？** 不会。Reference 在第一次失败前没有发布半成品；真实初始化应先本地构造，成功时发布。

**函数 static 能保护以后所有修改吗？** 不能，它只保护初始化。返回对象的后续可变访问仍要同步，结束进程前也要停止访问者。

**为什么不用锁外普通指针双重检查？** 那个锁外读取并没有参与写者的 mutex 协议，可能存在数据竞争和未安全发布。

## 构建与验证

以下两套命令都在 `C08_Concurrency/exercises` 工作目录运行；Windows 示例使用 Release，构建目录分开以避免缓存选项混淆。

Reference 基准答案门禁（不运行 main）：

```powershell
cmake -S B3_call_once -B build/reference-B3_call_once -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/reference-B3_call_once --config Release --target B3_call_once_reference
ctest --test-dir build/reference-B3_call_once -C Release -R "^B3_call_once_reference$" --no-tests=error --output-on-failure
```

学生实现验收（实际运行 main）：

```powershell
cmake -S B3_call_once -B build/student-B3_call_once -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-B3_call_once --config Release --target B3_call_once
ctest --test-dir build/student-B3_call_once -C Release -R "^B3_call_once_student$" --no-tests=error --output-on-failure
```

原样 Starter 返回 1，student CTest 报 Failed 是预期；不能把它标为 SKIP 或 WILL_FAIL 来制造通过。默认 `CONCURRENCY_STUDY_TEST_STARTERS=OFF`，未完成学生测试不进入 Reference 门禁。可选直接有界运行：

```powershell
python tools/run_diagnostic.py --timeout 5 -- ./build/student-B3_call_once/Release/B3_call_once.exe
```

成功只说明本次输入与实际交错通过相应检查；文字推导、一般交错、未测平台仍须单独核验。规范和完整答案见正文；不要求特定加速。
