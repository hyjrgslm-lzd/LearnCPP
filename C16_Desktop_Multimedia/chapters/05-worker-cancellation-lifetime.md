# L05 Worker 线程、取消与对象寿命

本章的目标不是“会开一个线程”。桌面应用里更常见的问题是：用户切换媒体、关闭窗口、对象已经销毁，而后台分析还在路上。我们要让结果只在仍然有效的对象上生效，让取消有限结束，让关闭不把悬空回调留给下一次事件循环。

本章默认你已经理解 C++ RAII、智能指针、基本线程同步和 lambda 捕获。Qt 从这里开始讲：`QObject` 的线程归属、`QThread` 的双重身份、queued 调用、receiver/context 连接，以及为什么 GUI 对象只能由主线程拥有和更新。

## 1. 反向先修

后面的媒体工作台需要这些能力：

- 用户选中一个媒体文件后，后台读取帧、计算波形或缩略图。
- 用户很快切到另一个文件，旧分析结果必须丢弃。
- 用户关闭窗口时，后台线程必须收束，不能晚到访问已销毁控件。
- UI 线程不能被解码或分析阻塞。

这要求先掌握四件事：

1. `QObject` 属于某个线程。`object.thread()` 说的是它的 thread affinity，也就是 queued 调用和事件投递会在哪个线程执行。
2. `QThread` 对象本身通常活在创建它的线程；它管理的运行线程是另一件事。不要因为拿着 `QThread*` 就以为当前代码已经在那个线程。
3. queued 调用依赖目标线程的事件循环。如果 worker 正在一个长 slot 里阻塞，同一个 worker 上排队的 `cancel()` slot 不会插队执行。
4. 跨线程结果必须带 context/receiver 和 generation。context 管寿命，generation 管“这是不是当前请求”。

## 2. 一个正确但有限的同步基线

先看最小同步版本。用户点击“分析”，主线程直接扫帧并更新模型。下面这段是媒体场景的概念代码，`read_frames` 和 `compute_rms` 代表后面媒体单元会展开的真实解码与采样处理：

```cpp
AnalysisResult analyze_now(MediaItem item)
{
    AnalysisResult out;
    for (Frame frame : read_frames(item.path)) {
        out.rms.push_back(compute_rms(frame));
    }
    return out;
}
```

它的优点是对象关系简单：调用返回前，`item`、模型和 UI 都还在当前栈上。没有晚到结果，也没有跨线程 receiver。

本章的可运行同步基线在 `baseline.cpp`，它把真实媒体处理收缩成同一契约下的有限帧计数：输入有 `media_id` 和正帧数，输出必须保留 `media_id` 并计满每一帧。它还向 Qt 主线程投递一个 queued marker，然后立刻运行同步分析。检查点是：同步函数返回前 marker 没运行；回到事件循环后 marker 才运行。

运行：

```powershell
cmake --build build/c16-l05 --target c16_l05_baseline
ctest --test-dir build/c16-l05 -R c16_l05_baseline --output-on-failure
```

预期能看到类似 trace：

```text
sync baseline trace
  sync frame 0
  sync frame 1
  sync frame 2
  main queued marker ran
```

这个 witness 不声称实测媒体解码卡顿。它只证明同步主线程函数执行期间，投递给同一个主线程事件循环的 queued 事件不会插队执行。真实媒体帧读取和缩略图计算如果也放在主线程，就会继承同样的事件循环阻塞性质；耗时多长要由后续媒体单元用真实输入另行测量。

## 3. 第一版异步：把 worker 放到 QThread

Qt 常见写法是：

```cpp
auto* worker = new Worker;
worker->moveToThread(&thread);
connect(&thread, &QThread::finished, worker, &QObject::deleteLater);
thread.start();

QMetaObject::invokeMethod(worker, [=] {
    auto result = worker->analyze(path);
    QMetaObject::invokeMethod(receiver, [=] {
        model->apply(result);
    }, Qt::QueuedConnection);
}, Qt::QueuedConnection);
```

这里有两个容易误读的点。

第一，`thread` 这个 `QThread` 对象通常仍属于创建它的主线程。`thread.start()` 创建了一个运行线程，但你在主线程里调用 `thread.quit()`、`thread.wait()` 是正常的。真正搬到 worker 线程的是 `worker`，因为调用了 `moveToThread`。

第二，`invokeMethod(worker, ..., Qt::QueuedConnection)` 只是把函数投递到 worker 所属线程的事件循环。它不会抢占正在运行的 worker 函数。worker slot 不返回，后面的 queued slot 就不能执行。

## 4. 受控复现：排队取消不会打断阻塞 worker

同一个 `baseline.cpp` 的第二半是 queued cancel 问题复现。它不靠 `sleep` 猜调度，而是用 `QSemaphore` 控制 worker 卡在明确位置：

```powershell
cmake --build build/c16-l05 --target c16_l05_baseline
ctest --test-dir build/c16-l05 -R c16_l05_baseline --output-on-failure
```

实验顺序是：

1. 主线程把 `analyze()` queued 到 worker。
2. worker 进入 `analyze()`，释放 `entered`，然后阻塞在 `resume.acquire()`。
3. 主线程把 `cancel()` queued 到同一个 worker。
4. 主线程处理自己的事件，确认 `cancel()` 没有运行。
5. 测试释放 `resume`，`analyze()` 返回。
6. worker 事件循环恢复，queued `cancel()` 才运行。

预期还能看到类似 trace：

```text
worker queued-cancel trace
  analyze entered worker thread
  cancel was still queued during work
  queued cancel slot ran
```

这个结果说明：如果取消逻辑本身只是排队给同一个被阻塞的 worker，它不能作为“停止按钮”。它最多在长任务结束后执行，已经太晚。

## 5. 改进：取消状态必须能被正在运行的 work 看到

练习的 `FrameGate` 模拟媒体分析里的“下一帧可继续”。正确方向是：

- 当前请求持有共享取消状态。
- `cancel()` 在调用线程设置这个状态，并释放可能阻塞的 gate。
- worker 每个安全检查点观察取消状态，有限返回。
- 关闭时也走同一个释放路径，然后等待线程结束。

关键区别是：取消不是“排队让 worker 以后记得取消”，而是“让正在运行的 worker 已经能看到取消”。

这并不等于可以随时杀线程。C++ 和 Qt 都不鼓励强杀工作线程，因为资源、锁、文件句柄和库内部状态都可能停在不变量中间。我们用协作取消：选择安全检查点，允许当前小步骤收束，然后返回。

## 6. generation：旧结果晚到也不能生效

用户从 `a.mp4` 切到 `b.mp4` 时，旧请求可能已经在 worker 里，无法瞬间消失。即使它被取消，也可能投递一个“已取消”结果。UI 只关心最新请求，所以 controller 需要单调 generation：

```cpp
const int request_id = ++generation_;
post_to_worker(request_id);

// 结果回到主线程时：
if (request_id != generation_) {
    return; // old result
}
```

这条检查必须在主线程应用结果前做。只在 worker 里检查不够，因为 worker 看到的状态和 UI 当前选择之间有时间差。

练习里的 bad 变体故意遗漏 generation 拒旧。检查器会构造旧请求先进入 worker、新请求替换它、旧请求晚到的交错，并要求只收到新请求结果。

## 7. receiver/context：对象死了就别回调

Qt 的连接通常有 receiver/context：

```cpp
connect(worker, &Worker::finished, receiver, [receiver](Result r) {
    receiver->apply(r);
});
```

receiver 销毁后，Qt 会移除以它为接收者的连接。使用 functor 投递时，也要给 `QMetaObject::invokeMethod` 一个 context，或者用 `QPointer` 在执行前检查对象是否还活着。

本章练习使用 `QObject* receiver` 和 `QPointer`。这个小接口只支持 controller 和 receiver 同属一个线程，因为示例把最终 callback 投回 session 所在线程；`analyze()` 会拒绝跨线程 receiver。真实项目如果要支持跨线程 receiver，应把最终 callback 的 context 改为 receiver，并重新定义 model 更新发生在哪个线程。

结果返回主线程时先检查：

- session 自己是否还活着。
- receiver 是否还活着。
- session 是否仍接受结果。
- result generation 是否等于当前 generation。

这些检查各管一件事，不能互相替代。receiver 存活不代表请求仍是当前；generation 正确也不代表窗口还没关闭。

## 8. 安全关闭

关闭路径必须做到有限、可重复：

1. 设置“不再接受结果”。
2. 设置当前请求的取消状态。
3. 释放可能阻塞 worker 的 gate。
4. 请求线程事件循环退出。
5. `wait()` 等 worker 结束。
6. 第二次 `close()` 直接返回。

析构函数调用 `close()` 是合理的 RAII；但析构函数不能假设 worker 会自己很快停下。它必须解除阻塞，否则析构就可能永久等待。

`close()` 后的新 `analyze()` 应该明确失败。练习中返回 `-1`，因为它是教学用的小接口；真实应用可以选择 `expected`、错误信号或状态码。

## 9. 练习说明

位置：`C16_Desktop_Multimedia/exercises/L05_worker_lifecycle`。

已提供：

- `baseline.cpp`：观察同步帧计数基线阻塞主线程 queued 事件，以及 queued cancel 不能打断阻塞 worker。
- `checks.cpp`：行为检查器，实际调用所选 `solution.hpp`。
- `student/solution.hpp`：学生编辑入口，初始实现安全有限但失败。
- `reference/solution.hpp`：完整参考实现。
- `good/solution.hpp`：独立正确实现，不引用 Reference。
- `bad/solution.hpp`：真实错误实现，遗漏 generation 拒旧。

学生只编辑 `student/solution.hpp`。公开接口固定为：

```cpp
namespace c16_l05 {
struct FrameGate;
struct AnalysisResult;
class MediaAnalysisSession {
public:
    int analyze(QString media_id, int frames, std::shared_ptr<FrameGate> gate,
                QObject* receiver, Callback callback);
    void cancel() noexcept;
    void close() noexcept;
    bool isRunning() const noexcept;
};
}
```

Part 目标：

1. 正常完成：worker 线程处理两个受控 frame，结果回到主线程，`worker_thread != QThread::currentThreadId()`。
2. 协作取消：worker 阻塞在第一帧时，`cancel()` 必须让它有限返回，且不提交该帧。
3. generation 拒旧：旧请求晚到不能调用回调；只接受新请求结果。
4. receiver 寿命：receiver 提前销毁后，旧回调不运行；后续 live receiver 仍可收到结果。
5. 安全关闭：飞行中关闭不调用回调，线程结束，重复关闭安全，关闭后拒绝新工作。

## 10. 完整解析

最小正确实现有三个状态：

- `current_gate_`：当前请求的取消和解除阻塞入口。
- `generation_`：主线程上的最新请求编号。
- `accepting_ / closed_`：关闭后拒绝新请求和飞行结果。

`analyze()` 先检查 receiver 与 controller 同线程，再取消旧请求、递增 generation，然后把 work queued 到 worker。worker 完成后不要直接碰 UI；它再把结果 queued 回 session 所在线程。session 线程里检查 receiver、accepting 和 generation，通过后才执行 callback。

`cancel()` 不调用 `QThread::requestInterruption()` 作为普通取消。普通取消后 session 还要接受下一次请求；线程级 interruption 更适合关闭。练习实现中普通取消只设置 gate 的取消位并释放 gate。`close()` 再请求线程退出并等待。

`bad/solution.hpp` 的错误很典型：它把回调投递给 receiver，但没有在主线程检查 result generation。receiver 还活着时，旧请求的取消结果仍会调用回调，污染当前 UI 状态。检查器的 `old -> new -> old late` 交错会拒绝它。

## 11. 源码阅读入口

阅读 Qt 源码时分清 public API 和 private 实现。本章建议固定 Qt 6.9.2：

- `QObject::moveToThread`：关注 thread affinity 改变后，queued 调用投递到哪里。
- `QMetaObject::invokeMethod` 的 functor overload：关注 context 对象销毁时，投递调用如何失效。
- `QThread::start/quit/wait`：关注 `QThread` 对象和运行线程的区别。
- `QObject::deleteLater`：关注删除动作依赖事件循环；线程收束时为什么常连接 `QThread::finished`。

源码阅读的问题不是背实现细节，而是验证本章模型：事件投递靠 receiver/context 和 thread affinity；正在执行的 slot 不会被后续 queued slot 抢占；对象寿命和请求新旧是两条独立边界。
