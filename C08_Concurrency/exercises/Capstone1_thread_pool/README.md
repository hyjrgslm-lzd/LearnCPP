# Capstone1：有界异步线程池

先读 [正文](../../chapters/06-cancellation-and-shutdown.md)，再对照 [完整 Reference](solution.cpp)。本题默认 C++23，所有检查使用 Release 下仍有效的 cs::check；Starter 默认在启动并发前以 1 退出，学生实现的等待由外部 CTest 超时限制。

## 必做 Parts

1. 说明 submit→队列→worker→future 的执行与所有权流；用 worker ID 与提交者 ID 不同验证真实异步。
2. 复用有界通道，以 move_only_function 包装 packaged_task；支持 move-only、void、成员函数和显式 ref 借用。
3. 令 invoke_result 使用衰减后实际存储与右值调用类型，验证有 &/&& 不同返回类型的函数对象。
4. 容量 1、唯一 worker 被 gate 占用、队列再有一项时，新的 submit 等待；shutdown 拒收该提交并唤醒，然后释放 gate 排空已接受任务。
5. 实现公开幂等 shutdown，允许外部并发关闭，拒绝新提交，排空并 join；验证析构后 future 仍可取值。
6. 检查 300 个独立任务结果与逐 ID 执行次数，检查任务异常经 future 到达且 worker 后续继续执行。
7. 拒绝零 worker/容量，同池 worker submit/shutdown 提前报错；解释自销毁禁止、嵌套依赖死锁和部分线程创建失败回滚。

## Reference 对照与运行观察

solution.cpp 与 runtime_tests/thread_pool_test.cpp 都调用本题 checks.hpp 中的 `pool_checks::run<cs::thread_pool>()`。三个类型参数化场景为 results_and_types、close_full_pool、ownership_and_shutdown；算法仍位于公共 thread_pool.hpp。

## 学生入口与检查

[main.cpp](main.cpp) 提供独立 Starter，按中文 TODO 完成对应学生函数或方法；不要直接包含 solution.cpp。完成一个 Part 后把该项 `part*_done` 改为 true。标记仅解除启动保护，不代替验收：只有实际调用学生实现的检查通过才返回 0。未完成时输出 `STARTER INCOMPLETE` 并返回 1，发生在任何线程创建之前。

在 `student_pool` 中完成七个 Part：构造与回滚、worker_loop、submit 包装及类型、背压、shutdown/drain/join、异常结果与生命周期。允许复用已学的 `cs::bounded_channel`，池本身须独立实现。`pool_checks::run<student_pool>()` 检查学生池；Reference 与 runtime 用 [checks.hpp](checks.hpp) 实例化 `cs::thread_pool`。检查头不包含任何池实现。

改动 main.cpp 后必须运行下面的 student 命令。Reference 的通过不说明学生实现正确。若只改标记、没有补全函数，TODO 或检查仍会失败；错误同步协议也可能被 CTest 的 30 秒超时终止。

完整推导、边界与所有复盘答案在 [同步专题 01](../../topics/synchronization/01-bounded-thread-pool.md)。公共头仅依赖 C++23 标准库及 Threads；不需要定制多源或第三方链接。

## 复盘答案

**为什么不能同步执行冒充线程池？** 它改变执行线程与阻塞行为，worker ID 检查会直接失败；结果相等不代表满足异步契约。

**容量是否包括执行中任务？** 不包括，只限制排队任务；外部阻塞提交者也可能持有任务内存。

**shutdown 与 cancel-pending 是一回事吗？** 不是，本题只提供 drain。取消待执行任务必须另定 future 的终态和任务资源回收协议。

**同池 worker 为什么不能 submit？** 有界满队列可耗尽全部 worker；即使队列未满，父任务等待子任务也可能形成资源环。本实现明确拒绝这种调用。

**部分构造失败由析构修复吗？** 不完整对象不调用自身析构，所以构造 catch 必须 close 并 join 已创建线程；源码已实现，资源故障注入尚未实测。

## 构建与验证

以下两套命令都在 `C08_Concurrency/exercises` 工作目录运行；Windows 示例使用 Release，构建目录分开以避免缓存选项混淆。

Reference 基准答案门禁（不运行 main）：

```powershell
cmake -S Capstone1_thread_pool -B build/reference-Capstone1_thread_pool -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/reference-Capstone1_thread_pool --config Release --target Capstone1_thread_pool_reference
ctest --test-dir build/reference-Capstone1_thread_pool -C Release -R "^Capstone1_thread_pool_reference$" --no-tests=error --output-on-failure
```

学生实现验收（实际运行 main）：

```powershell
cmake -S Capstone1_thread_pool -B build/student-Capstone1_thread_pool -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-Capstone1_thread_pool --config Release --target Capstone1_thread_pool
ctest --test-dir build/student-Capstone1_thread_pool -C Release -R "^Capstone1_thread_pool_student$" --no-tests=error --output-on-failure
```

原样 Starter 返回 1，student CTest 报 Failed 是预期；不能把它标为 SKIP 或 WILL_FAIL 来制造通过。默认 `CONCURRENCY_STUDY_TEST_STARTERS=OFF`，未完成学生测试不进入 Reference 门禁。可选直接有界运行：

```powershell
python tools/run_diagnostic.py --timeout 5 -- ./build/student-Capstone1_thread_pool/Release/Capstone1_thread_pool.exe
```

成功只说明本次输入与实际交错通过相应检查；文字推导、一般交错、未测平台仍须单独核验。规范和完整答案见正文；不要求特定加速。
