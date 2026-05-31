# 练习 4：scheduler 是能力，不是线程

## 目标

通过同一个 scheduler 承载多项工作，观察"scheduler 不等于某一条线程"，而是"把工作安排到某类执行资源上的入口"。

## 前置理解

- 你知道如何创建 `exec::static_thread_pool`。
- 你知道 `pool.get_scheduler()` 会返回一个 scheduler 对象。
- 你理解本题是观察调度语义，不是做线程池 benchmark。

## 必做任务

1. 创建一个 `exec::static_thread_pool`，先用 1 个工作线程做一遍，再用 4 个工作线程做一遍。
2. 从池中获取 scheduler。
3. 构造至少 8 个独立 sender，每个 sender 都从 `schedule(sch)` 开始，并在 `then` 中打印自己的任务编号和线程 ID。
4. 用 `when_all` 或其它你熟悉的组合方式等待这些 sender 全部完成。
5. 记录 1 线程池版本和 4 线程池版本的线程 ID 分布。
6. 画出对象关系：`pool -> scheduler -> sender -> operation_state`。

## 进阶任务

- 在每个任务里加入极短的 `sleep_for`，让线程分布更容易观察。
- 让每个任务多做两段计算，比较"同一 scheduler 发出的不同任务"是否必然落在同一线程上。
- 用同一个 scheduler 连续创建两批任务，观察 scheduler 可复制、可重复使用这一点。

## 验收点

- 你能明确说明 scheduler 不是线程对象。
- 你能解释为什么同一个 scheduler 在多线程池版本下可能让不同任务跑在不同线程上。
- 你能说明 `pool` 与 `scheduler` 的拥有关系。
- 你能说清 sender 并不是线程池本身，而是"使用这个 scheduler 的工作描述"。

## 观察点

- 1 线程池下，你通常会看到所有任务都落在同一工作线程。
- 4 线程池下，你可能看到多个线程参与，但这并不意味着你拿到了多个 scheduler。
- scheduler 的身份更接近"队列入口"或"调度能力句柄"。

## 常见坑

- 把 `pool.get_scheduler()` 当作"拿到一个线程"。
- 任务太短，导致 4 线程池看起来像只用了一个线程，于是误判 scheduler 行为。
- 直接在主线程调用普通函数打印，混淆了 `schedule(sch)` 后的观察结果。
- 把"运行在哪个线程"当作 scheduler 唯一价值，忽略了它是图里的调度边界。

## 提示

- 任务数最好比线程数多，例如 8 到 16 个。
- 如果输出太乱，就给每条日志加上任务编号与阶段前缀。
- 不要追求线程分布绝对平均，只要能观察到"同一 scheduler 不等于单线程实体"就够了。
- 如果你习惯先写函数，再把函数挂到 sender 上，会更容易看清"工作内容"和"调度入口"的分离。

## 复盘问题

- 为什么 scheduler 可以复制和复用，但不会凭空复制出更多线程？
- 如果把线程池从 4 改成 8，你是在改 sender 图，还是在改执行资源能力？
- 为什么 `schedule(sch)` 更像"请在这个执行资源上开始一段工作"，而不是"马上切到某线程执行函数"？
- 这题里，谁拥有真正的一次执行实例？

## 对应官方参考

- `stdexec/examples/hello_world.cpp`
- `NVIDIA/stdexec` README 中 `static_thread_pool` 的基础示例
