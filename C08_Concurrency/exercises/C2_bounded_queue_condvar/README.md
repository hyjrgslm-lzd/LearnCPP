# C2：有界 MPMC 通道的 FIFO 与关闭

先读 [正文](../../chapters/05-waiting-and-channels.md)，再对照 [完整 Reference](solution.cpp)。本题默认 C++23，所有检查使用 Release 下仍有效的 cs::check；Starter 默认在启动并发前以 1 退出，学生实现的等待由外部 CTest 超时限制。

## 必做 Parts

1. 在 main.cpp 的 student_channel 中实现 push/pop 的锁内完整操作；等待满/空及关闭谓词，分别通知对侧。
2. 拒绝零容量，验证容量 1 与 2、环绕复用及 unique_ptr 元素；closed push 失败，closed pop 继续排空。
3. 三生产者各生成 200 个唯一 ID，分别以一消费者、两消费者运行。检查全部 ID 恰好出现一次；单消费者额外检查每个生产者的内部顺序。
4. 满队列上发起多次 push、空队列上发起多次 pop，close 广播两侧并等待所有调用结束。重复 close 不改变契约。

## Reference 对照与运行观察

`checks.hpp` 的 `sequential<Channel>()`、`concurrent<Channel>(1/2)`、`close_waiters<Channel>()` 给两种独立实现使用同一套契约检查；solution.cpp 选择公共答案类型，main.cpp 选择学生类型。

## 学生入口与检查

[main.cpp](main.cpp) 提供独立 Starter，按中文 TODO 完成对应学生函数或方法；不要直接包含 solution.cpp。完成一个 Part 后把该项 `part*_done` 改为 true。标记仅解除启动保护，不代替验收：只有实际调用学生实现的检查通过才返回 0。未完成时输出 `STARTER INCOMPLETE` 并返回 1，发生在任何线程创建之前。

在 `main.cpp` 的 `student_channel<T>` 中完成四个 Part：构造/槽位与容量、push/pop 等待及 MPMC 互斥、close 拒收与排空。`channel_checks::run<student_channel>()` 检查学生类型；`solution.cpp` 使用同一个 [checks.hpp](checks.hpp) 实例化 `cs::bounded_channel`。检查头不包含公共 channel 实现，学生入口也不包含它。

改动 main.cpp 后必须运行下面的 student 命令。Reference 的通过不说明学生实现正确。若只改标记、没有补全函数，TODO 或检查仍会失败；错误同步协议也可能被 CTest 的 30 秒超时终止。

T 必须无抛出移动构造、无抛出析构，且这些操作不能重入同一通道。队列固定槽位在构造时分配，普通操作不增加内部容器存储。

## 复盘答案

**为什么 pop 不先检查 empty 再另行拿元素？** 两步间其他消费者可能取走对象，破坏访问前提。检查、移动、移除必须在一个临界区。

**false push 是否保留调用者右值？** 不保证，参数按值接收可能在进入函数前就已移动；false 仅表示没有被通道接受。

**close 后 nullopt 意味什么？** 关闭并已排空；开放的空队列会等待，关闭但非空仍返回元素。

**两消费者的结果合并是否代表 FIFO 历史？** 不代表，记录/返回调度可能不同于持锁出队顺序。本题检查集合与单消费者顺序，并给出锁内线性化推导。

**close 后能马上析构吗？** 不能，必须先结束并 join 全部访问者；广播返回不代表被唤醒者已停止访问。

## 构建与验证

以下两套命令都在 `C08_Concurrency/exercises` 工作目录运行；Windows 示例使用 Release，构建目录分开以避免缓存选项混淆。

Reference 基准答案门禁（不运行 main）：

```powershell
cmake -S C2_bounded_queue_condvar -B build/reference-C2_bounded_queue_condvar -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/reference-C2_bounded_queue_condvar --config Release --target C2_bounded_queue_condvar_reference
ctest --test-dir build/reference-C2_bounded_queue_condvar -C Release -R "^C2_bounded_queue_condvar_reference$" --no-tests=error --output-on-failure
```

学生实现验收（实际运行 main）：

```powershell
cmake -S C2_bounded_queue_condvar -B build/student-C2_bounded_queue_condvar -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-C2_bounded_queue_condvar --config Release --target C2_bounded_queue_condvar
ctest --test-dir build/student-C2_bounded_queue_condvar -C Release -R "^C2_bounded_queue_condvar_student$" --no-tests=error --output-on-failure
```

原样 Starter 返回 1，student CTest 报 Failed 是预期；不能把它标为 SKIP 或 WILL_FAIL 来制造通过。默认 `CONCURRENCY_STUDY_TEST_STARTERS=OFF`，未完成学生测试不进入 Reference 门禁。可选直接有界运行：

```powershell
python tools/run_diagnostic.py --timeout 5 -- ./build/student-C2_bounded_queue_condvar/Release/C2_bounded_queue_condvar.exe
```

成功只说明本次输入与实际交错通过相应检查；文字推导、一般交错、未测平台仍须单独核验。规范和完整答案见正文；不要求特定加速。
