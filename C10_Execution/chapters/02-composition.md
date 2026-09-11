# 02 composition: 分支、合流与惰性边界

组合器让异步框架避免共享可变对象。主线例子是一条记录流水线：原始记录进入 parse，得到结构化值；多个独立分支各自产生 profile、quota、flags；合流点组装 dashboard 或 report。

## `then`: 改写 value channel

`then(f)` 不立即调用 `f`。它包装 receiver，只在上游发出 `set_value(args...)` 时调用 `f(args...)`，再把返回值继续发给下游。若 `f` 返回 `void`，下游收到无参 `set_value()`；若 `f` 抛异常，教学实现把它转成 `set_error(std::exception_ptr)`。

这和普通函数调用的区别是时序：

```text
构造: just(5) | then(+1) | then(*2)
连接: connect(sender, receiver) -> operation state
启动: start(op) -> value 5 -> +1 -> *2 -> receiver
```

A2 的日志就是这个时序的可运行证据。

## `when_all`: 合流不是自动并行

`when_all(a, b, c)` 表示三个 sender 都完成后，把它们的 value 一起交给下游。它不保证创建线程；如果三个分支都没有 scheduler，它们可以在同一调用链里运行。并行来自执行资源和调度边界，不来自 `when_all` 这个词。

A1 的四段求和展示“图形状”：四个局部 sum 合成总 sum。checker 记录每个输入下标的访问次数，修掉旧版本最后一段从 `2*chunk` 开始的重叠 bug。A3 展示业务形状：profile、quota、flags 分支不写共享 dashboard，最后一个 `then` 才构造完整对象。

## 值归属

组合边界最好用值传递表达所有权。每个分支生产自己的结果，合流点消费这些结果。这样后续加入 error、stopped、scope、retry 时，哪个阶段拥有哪个对象会清楚很多。

坏味道是提前创建一个大对象，然后多个分支写字段。那会把 execution 题变成共享状态题；你必须额外证明锁、生命周期、失败时半成品状态。C10 的基础题先避免这条路，等 C08 的同步规则和后续 scope 收束一起再处理。

## 和 C++17 并行算法的边界

并行算法适合“对一个范围应用一个算法”。sender 图适合“描述多个异步来源、完成通道、调度位置和合流关系”。A1 同时保留两者，是为了让差异可见：并行策略更短，sender 图更显式。
