# 结课1 · 线程池与生产者-消费者

> 详尽版见 `../../06-第一阶段结课-线程池与生产者消费者.md`（项目目标 / 固定题面 / 设计约束 / 推荐输入输出 / 必做任务 / 进阶任务 / 验收点 / 复盘问题）。

## 目标

综合阶段一全部核心原语，实现一个**固定大小线程池** `ThreadPool`：

- `std::jthread`（C++20）持有 N 个常驻 worker，析构自动 join；
- `std::mutex` + `std::condition_variable` 维护受保护的（建议有界）任务队列；
- `submit(callable, args...)` 用 `std::packaged_task` 打包任务，返回 `std::future<R>` 回传结果与异常；
- `std::stop_token`（C++20）驱动优雅停机：先停收新任务，排空在途任务再退出。

本质就是把练习 **C-2（有界阻塞队列）+ C-3（stop_token 可中断等待）+ D-3（packaged_task）** 缝合成一个可复用组件。

## 前置理解

- 练习 C-2：一把锁 + 条件变量的有界阻塞队列，及 `notify_one` vs `notify_all`、惊群。
- 练习 C-3：`condition_variable_any` + `stop_token` 的可中断 `wait`。
- 练习 D-3：`packaged_task` 如何解耦“结果/异常”与“执行”。

## 必做任务（详见根目录文档）

1. 画提交端→队列→worker→future 的数据流图。
2. 定义 `void()` 任务类型与（有界）队列。
3. 实现 `submit`：`packaged_task` 打包、先取 `future` 再入队。
4. 实现 worker 取任务-执行循环（**先解锁再执行**，禁止忙等）。
5. 实现加锁入队/出队与通知（含背压）。
6. 实现优雅停机：停机标志 + `notify_all`，排空后退出，绝不丢弃在途任务。
7. 测试驱动：返回值路径（一批平方任务求和比对）。
8. 测试驱动：异常路径（`future.get()` 接住任务抛出的异常）。

## 进阶任务

- 用 `stop_token` + `condition_variable_any` 取代自定义停机布尔。
- 提供 drain（排空后退出）/ cancel（丢弃未执行）两种停机策略。
- 阻塞式背压 vs 拒绝式背压；`submit` 支持成员函数与移动捕获 lambda；加观测计数器。

## 验收点

- 返回值求和与期望**完全一致**、稳定可复现。
- 任务异常经 `future.get()` 重新抛出，不让 worker 崩溃。
- 析构无任务丢弃（提交数 == 完成数）；worker 空闲时**睡眠**而非忙等。
- ThreadSanitizer 无数据竞争报告（MSVC 下以逻辑审查替代）。

## 骨架现状

`main.cpp` 为可编译运行的骨架：关键实现以 `// TODO [必做 N]:` / `// TODO [进阶 N]:` 标注，参考代码以注释给出。未填 TODO 时线程池退化为“提交线程内同步执行任务”的占位实现，输出仍正确（但无真正并发）。按 TODO 把占位段替换为参考实现即可获得真正的并发线程池。

## 编译运行（VS2026, C++20）

```bash
cmake --build build-vs2026 --target Capstone1_thread_pool --config Release
```

## 对应官方参考

- 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 9 章（线程池）
- cppreference：`jthread` / `packaged_task` / `condition_variable` / `stop_token`
