# L05 worker lifecycle

本练习对应 `chapters/05-worker-cancellation-lifetime.md`。

## Part 1: 正常结果回到主线程

编辑 `student/solution.hpp`，实现 `MediaAnalysisSession::analyze`。它必须把实际帧处理放到 Qt worker 线程，结果通过接收者所在的 Qt 事件循环回调。

## Part 2: 协作取消

`cancel()` 不能排队到正在阻塞的 worker slot 上。它应从调用线程设置共享取消状态，并释放受控 gate，让 worker 在下一个检查点有限退出。

## Part 3: generation 拒旧

新的媒体请求会替代旧请求。旧请求即使晚到，也不能更新 UI 或调用接收者回调。

## Part 4: receiver 提前销毁

回调必须绑定 receiver/context。receiver 已销毁时，结果丢弃，不访问悬空对象。

## Part 5: 安全关闭

`close()` 可重复调用；关闭时拒绝新 work，解除阻塞，等待 worker thread 结束，并丢弃飞行中的结果。

运行：

```powershell
cmake -S C16_Desktop_Multimedia/exercises/L05_worker_lifecycle -B build/c16-l05 -G Ninja -DCMAKE_PREFIX_PATH=D:/Qt/6.9.2/6.9.2/msvc2022_64
cmake --build build/c16-l05
ctest --test-dir build/c16-l05 --output-on-failure
```
