# 迁移入口：future 与异步任务

本模块现在先讲结果，再讲执行：

1. [02 结果通道](chapters/02-result-channels.md)：单线程 P2 解释 valid、ready、wait、get、异常、broken_promise 和 shared_future。
2. [03 线程与执行方式](chapters/03-threads-and-execution.md)：把结果通道放到线程之间，再学习显式 async 策略和任务打包。
3. [D1 promise/future](exercises/D1_promise_future/README.md)、[D2 async](exercises/D2_async_policies/README.md)、[D3 packaged_task](exercises/D3_packaged_task/README.md)：各自提供安全 Starter、完整 Reference、Part 答案和运行命令。

不再用 sleep 或指定加速比验收。async 不承诺函数体立即获得运行机会；保存 future 允许任务重叠，但不保证提速。析构等待需要分析共享状态来源和最后引用释放；持有 move-only 对象的闭包不能直接装入 std::function。D3 使用真实 packaged_task<void()> 封装，线程池实现留给后续专题。
