# H1：一次完成、重复阶段与减员

先读 [正文](../../chapters/07-coordination.md)，再对照 [完整 Reference](solution.cpp)。本题默认 C++23，所有检查使用 Release 下仍有效的 cs::check；Starter 默认在启动并发前以 1 退出，学生实现的等待由外部 CTest 超时限制。

## 必做 Parts

1. 用 ready(4) 收集四份准备结果，用 go(1) 阻止 worker 提前开始；检查全部准备先于发令，发令后四人完成。
2. 三个 worker 各写独立槽位，四轮 barrier completion 汇总，检查 33、63、93、123 且完成次数为四。
3. 拆开 arrive 和 wait，说明 arrival_token 所属 phase 以及二者之间允许的访问。
4. 两参与者中一人 arrive_and_drop，下一轮仅一人即可完成；检查当前与未来 expected count 都改变。
5. 在创建多轮线程前使用 launch gate，部分启动失败时让已创建者在进入 barrier 前退出。

## Reference 对照与运行观察

`latch_start()` 和 `barrier_phases()` 是两个独立 Reference 场景，后者含 split arrival、drop 与启动失败回滚。completion 只写固定结果，不抛检查异常。

## 学生入口与检查

[main.cpp](main.cpp) 提供独立 Starter，按中文 TODO 完成对应学生函数或方法；不要直接包含 solution.cpp。完成一个 Part 后把该项 `part*_done` 改为 true。标记仅解除启动保护，不代替验收：只有实际调用学生实现的检查通过才返回 0。未完成时输出 `STARTER INCOMPLETE` 并返回 1，发生在任何线程创建之前。

Part 1 实现 `student_ready_and_wait`；Part 2 实现 `student_completion::operator()`；Part 3 实现 `student_arrive_then_wait`；Part 4 实现 `student_drop`；Part 5 实现 `student_allow_launch`。检查准备总和 10、放行前 started=0、四轮 33/63/93/123、减员后的两轮完成，以及 false 启动门禁止进入 phase。

改动 main.cpp 后必须运行下面的 student 命令。Reference 的通过不说明学生实现正确。若只改标记、没有补全函数，TODO 或检查仍会失败；错误同步协议也可能被 CTest 的 30 秒超时终止。

completion 必须满足无抛出调用要求；业务检查放到 future.get 之后。创建异常的回滚已实现，但本机未注入线程创建失败。

## 复盘答案

**latch 归零后能再次 count_down(1) 吗？** 不能，会超过当前计数；可以重复 wait，但不能重置。try_wait 允许虚假 false，不能要求一次成功。

**barrier 总由最后一个 arrive 的线程完成吗？** N5050 不这样保证。完成步骤可在 arrive、drop 或 wait 调用期间执行；没有任何 wait 时是否执行是实现定义的。本实验每轮都有 wait。

**为什么 slots 不需要 atomic？** 每线程写不同位置，写在 arrive 前，completion 在所有到达后读，下一轮写在 wait 返回后；共享访问之间有阶段同步。

**退出线程为何必须 drop？** 否则以后还欠它的到达，其他人可能永远等下去。drop 同时改变本轮和后续轮次的计数。

**发令后必须同时运行吗？** 不保证物理同时性，只保证未发令不能通过。

## 构建与验证

以下两套命令都在 `Concurrency_Study/exercises` 工作目录运行；Windows 示例使用 Release，构建目录分开以避免缓存选项混淆。

Reference 基准答案门禁（不运行 main）：

```powershell
cmake -S H1_latch_barrier -B build/reference-H1_latch_barrier -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/reference-H1_latch_barrier --config Release --target H1_latch_barrier_reference
ctest --test-dir build/reference-H1_latch_barrier -C Release -R "^H1_latch_barrier_reference$" --no-tests=error --output-on-failure
```

学生实现验收（实际运行 main）：

```powershell
cmake -S H1_latch_barrier -B build/student-H1_latch_barrier -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-H1_latch_barrier --config Release --target H1_latch_barrier
ctest --test-dir build/student-H1_latch_barrier -C Release -R "^H1_latch_barrier_student$" --no-tests=error --output-on-failure
```

原样 Starter 返回 1，student CTest 报 Failed 是预期；不能把它标为 SKIP 或 WILL_FAIL 来制造通过。默认 `CONCURRENCY_STUDY_TEST_STARTERS=OFF`，未完成学生测试不进入 Reference 门禁。可选直接有界运行：

```powershell
python tools/run_diagnostic.py --timeout 5 -- ./build/student-H1_latch_barrier/Release/H1_latch_barrier.exe
```

成功只说明本次输入与实际交错通过相应检查；文字推导、一般交错、未测平台仍须单独核验。规范和完整答案见正文；不要求特定加速。
