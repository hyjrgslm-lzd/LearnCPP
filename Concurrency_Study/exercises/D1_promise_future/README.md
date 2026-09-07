# D1：跨线程结果、异常和广播

先完成 [P2](../P2_result_states/README.md)，再读 [03 线程与执行方式](../../chapters/03-threads-and-execution.md)。[main.cpp](main.cpp) 是单 worker 返回 42 的安全 Starter；[solution.cpp](solution.cpp) 检查四个必做 Part。

## Part 与答案

### Part 1：等待结果，不等于线程已结束

在 worker 设值前写 published_input=21，设值为它的两倍。对应 `part1_and_2_transfer(false)`。主线程 wait 成功后读到 21，再 join，之后检查最终错误槽并 get=42。

前一次读取靠结果状态的同步边；错误槽的读取靠 join。若 worker 设值后仍修改 published_input，原来的读取证明就不成立。不要把该例泛化成“future ready 后可读 worker 的所有变量”。

### Part 2：传送原始业务失败

对应同一函数的 true 分支。worker 抛 calculation_failure，在 catch 中 set_exception，get 重抛相同类型与 cannot calculate 消息。即使状态里是异常，设值前的普通写仍通过成功等待发布。普通 future 在异常 get 后也无效。

结果传输操作失败另存 transport_error；必须 join 后检查。预期的业务异常与验证错误不混用，任何意外错误都使程序失败。

### Part 3：三个消费者各自拿一份 shared_future

对应 `part3_broadcast()`。先 share，再将句柄按值复制给三个消费者；生产者设 100。每个消费者重复 get，并写自己专属的数组元素与错误槽。全部 join 后逐项验证三个 100。

消费者容器比 provider 先声明。若创建第二/第三个线程时异常展开，provider 先放弃状态、解除等待，再析构线程容器完成 join。这个顺序是失败路径协议，不能随意交换。

### Part 4：提前返回与 void 完成事件

对应 `part4_abandon_and_event()`。把 promise 作为拥有值的线程参数，worker 正常返回但未设值，参数销毁使 get 抛 broken_promise。再创建 promise<void>，worker set_value，主线程 join 后检查错误并 get，句柄失效。

## 验收与适用边界

输出四段结果说明与 `D1_reference OK`。所有业务值和消费者槽逐项检查，不靠“收到三行日志”。无 sleep、无时间阈值；创建失败与 worker 检查异常有收尾路径。

这个通道只能完成一次，不支持流式发送、多次更新、取消或自动停止 worker。多个消费者得到同一值，不代表任意共享指针指向对象可以无锁修改。

规范：[共享状态](https://eel.is/c++draft/futures.state)、[shared_future](https://eel.is/c++draft/futures.shared.future)。固定版 N5050 与滚动页区分见正文。

## 构建与运行

从 `Concurrency_Study/exercises` 执行（VS2026 生成器需要 CMake 4.2+）：

```powershell
cmake -S D1_promise_future -B build/D1_promise_future -G "Visual Studio 18 2026" -A x64
cmake --build build/D1_promise_future --config Release
./build/D1_promise_future/Release/D1_promise_future.exe
ctest --test-dir build/D1_promise_future -C Release --output-on-failure
```

统一构建注册的检查目标是 `D1_promise_future_reference`，CTest 使用进程级超时。
