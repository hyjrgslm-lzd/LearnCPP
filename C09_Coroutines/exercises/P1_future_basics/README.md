# 练习 P-1：异步结果基础

先阅读 [预备章的 future 结果通道](../../00-预备知识-执行模型与标准库.md#future-model)，随后逐个完成 `main.cpp` 的五个作用域。`solution.cpp` 给出完整实验和结果检查。

| Part | 本次操作 | 对应讲解 | 参考结果 |
| --- | --- | --- | --- |
| P1-1 | 提供 42，比较 wait/get 前后的状态 | [结果通道](../../00-预备知识-执行模型与标准库.md#future-model) | `value=42 valid_after_get=false` |
| P1-2 | 通过 promise 放行 async 工作，记录线程 | [执行策略与完成信号](../../00-预备知识-执行模型与标准库.md#async-launch) | 放行前 pending，结果为 42，工作线程不同 |
| P1-3 | 提供异常，在 get 处捕获 | [异常传递](../../00-预备知识-执行模型与标准库.md#future-errors) | 收到异常，future 已消费 |
| P1-4 | 观察 deferred 查询状态与 get | [延迟求值](../../00-预备知识-执行模型与标准库.md#deferred-shared) | 由 get 在当前线程执行 |
| P1-5 | 复制 shared_future，再重复取值 | [共享读取](../../00-预备知识-执行模型与标准库.md#shared-future) | 两个入口均读取 7 |

在 `C09_Coroutines/exercises` 配置并构建：

```powershell
cmake --preset verify-core
cmake --build --preset verify-core --target P1_future_basics P1_future_basics_reference
```

Visual Studio 默认构建目录中的运行方式：

```powershell
./build/verify-core/P1_future_basics/Release/P1_future_basics.exe
./build/verify-core/P1_future_basics/Release/P1_future_basics_reference.exe
```

其他 CMake 生成器的可执行文件位置见 [构建指南](../BUILD_GUIDE.md)。Starter 的占位值帮助你先跑通观察过程；完成 TODO 后，逐项对照上表。Reference 最后输出 `P1_reference OK`。

完成后，用“提供方 → 共享状态 → 消费方”画出 Part 2 与 Part 3 的关系，并指出等待发生在哪个调用。

**答案解析：** Part 2 包含两条结果通道。第一条是 `release promise → void 共享状态 → producer 捕获的 future`，producer 在 `gate.wait()` 等待 main 放行。第二条是 `async producer → 计算结果共享状态 → main 持有的 result future`；main 发出放行信号后，通过 `result.get()` 等待并取得结果。参考实现记录的计算值为 42，执行计算的线程与 main 不同。

Part 3 的路径是 `producer.set_exception(...) → 就绪的异常状态 → result.get() → main 的 catch`。这里异常已经在调用 get 前存入共享状态，所以 get 直接重新抛出；如果提供方稍后才设置异常，get 会先等待状态就绪。成功取值和重抛已保存异常都会完成这一次 future 消费，随后 `valid()` 为 false。

接着学习 [P2 generator 基础](../P2_generator_basics/README.md)，后续 [A3](../A3_co_await_future/README.md) 将使用这些知识适配协程等待。
