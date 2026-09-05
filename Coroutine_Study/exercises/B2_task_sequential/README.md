# 练习 B-2：task<T> 顺序异步组合

> 详尽版本见 `../../03-模块B-generator与task的使用.md` 的 `练习 B-2` 章节。
> 本 README 仅摘抄"目标 / 必做任务 / 验收点"。

## 目标

用 `lazy_task<T>` 串联多个伪异步操作（`fetch -> parse -> validate`），通过 `co_await`
把三个任务串起来，让最终结果自然流到 `co_return`。对比等效的回调金字塔写法，体会协程
如何让异步顺序看起来像同步代码。

## 必做任务

1. 写三个独立协程：`fetch_user(id) -> User`、`parse_profile(User) -> Profile`、
   `validate_profile(Profile) -> ValidatedProfile`。每个内部用 sleep 模拟耗时。
2. 写顶层 `process_user(id) -> lazy_task<std::string>`，三步 `co_await` 串起来，
   最后用 `std::format` 拼接结果。
3. main 中用 `sync_wait` 消费并打印。
4. 在每个协程入口 / `co_return` 前打日志，构造一条清晰的时间线。
5. 给 `fetch_user` 加 ~30% 概率抛异常，确认异常沿 `co_await` 链传到 `sync_wait`。
6. 写等效的回调嵌套版做对比：错误处理分散在每一层 lambda 中。
7. 画协程调用树：`co_await` 处的挂起 / 子协程 resume / 完成回到父帧。

## 进阶任务

- 三个 task 改成"同时启动 + 分别等待"的近似并发模式，思考 lazy_task 的局限。
- 顶层 `try-catch` 捕获异常后降级返回默认串。
- 把某步类型改成 `lazy_task<std::optional<T>>`，体会 `co_await` 链上类型变化。

## 验收点

- 日志按 `fetch -> parse -> validate` 顺序出现。
- 当 `fetch_user` 抛异常时，`parse / validate` 日志完全不出现，异常被 `sync_wait` 重新抛出。
- 你能说清回调版的"错误处理散落在每层"为什么是结构性问题。
- 你能指出每个 `co_await` 既是挂起点又是异常通道入口。

## Starter / Reference

- `main.cpp` 是练习骨架，保留 TODO 和可编译串联流程。
- `solution.cpp` 是可运行参考实现，`ctest --preset verify-core -C Release -R B2_task_sequential_reference`
  会校验 `fetch -> parse -> validate` 顺序、最终文本和异常短路传播。
