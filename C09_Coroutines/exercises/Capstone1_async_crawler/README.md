# Capstone 1：异步小爬虫

先读 [第一阶段结课讲义](../../05-第一阶段结课-异步小爬虫.md)。本项目把 A/B/C 的知识合在一起：fetch 用 `co_await`，parse 用 generator，多个 fetch 用 when_all 语义汇合，超时用 `stop_token`，动态任务用 scope 收束。

相关源码：

- [url_table.hpp](url_table.hpp)：固定三条模拟 URL、响应正文和延迟。
- [parser.hpp](parser.hpp)：按行 `co_yield Record` 的 parser 骨架。
- [main.cpp](main.cpp)：项目 starter，使用 `worker_group + timer_awaiter{workers, delay, token}` 形状。
- [solution.cpp](solution.cpp)：可运行参考。

## Part 1：先画拓扑

把 [main.cpp](main.cpp) 顶部注释补成实际调用图：

```text
main -> stop_source + watchdog
main -> sync_wait(aggregate)
aggregate -> co_await when_all_fetch
when_all_fetch -> scope.spawn(fetch_into * N) -> scope.join
aggregate -> parse_lines(body) generator -> report
```

每条边旁边写 owner：root task 由 `sync_wait` 消费；fetch task 由 scope 收束；timer worker 由 `worker_group` join；generator 由遍历它的对象持有。

**答案解析：** 合格拓扑图要同时画控制流和 owner：main 拥有 `stop_source`、`watchdog`、`worker_group`，`sync_wait` 消费 root `aggregate` task；`aggregate` 等 `when_all_fetch`，`when_all_fetch` 的 scope 拥有所有 fetch task 并在 `join()` 收束；每个 `timer_awaiter` 把等待交给 `worker_group`；aggregate 遍历 `parse_lines(body)` 时，generator 对象持有 parser 协程帧。stop token 从 main 复制到 aggregate、when_all_fetch、fetch_one，最后由 fetch_one 的检查点响应。

## Part 2：fetch 与超时

补 `fetch_one` 的两次 token 检查：

- `co_await` 前已经 stop，直接返回 `Stopped`。
- `co_await timer_awaiter{workers, rec.latency, st}` 后再检查，等待期间 stop 则返回 `Stopped`。

watchdog 在 200ms 后 `request_stop()`。三条 URL 中，100ms 和 150ms 的 fetch 应成功，300ms 的 fetch 应进入 stopped。

## Part 3：parser generator

补 [parser.hpp](parser.hpp)：跳过 header，按 `\n` 拆行，用逗号分出 name 和 score。`parse_lines(std::string body)` 按值接收 body，让 generator 帧持有响应内容；正确行 yield `ok=true`，错误行 yield `ok=false` 并继续。

这里要观察 generator 的惰性：aggregate 遍历一行，parser 才推进到下一行。项目数据很小，但这个结构以后可以扩展到流式解析。

## Part 4：when_all_fetch 与 scope

把串行占位改成并发：预分配 `std::vector<FetchResult> out(recs.size())`，每个 task 写自己的下标。用 scope 启动所有 `fetch_into(workers, rec, token, out, index)`，再 `join()` 等齐。

不要用共享 `push_back` 做结果收集，避免把练习重点变成容器并发保护。按下标写入足够简单。

## Part 5：aggregate 与报告

`aggregate` 拿到所有 fetch 结果后：

- `Ok`：调用 parser，累加成功行数和分数。
- `Stopped`：计入超时/停止。
- `Error`：计入失败。

最后输出总 URL 数、成功数、超时数、失败数、总解析行数、总分、整体耗时和每 URL 明细。

## 验收

参考实现 [solution.cpp](solution.cpp) 的核心结果：

- URL 总数 3。

  **答案解析：** 总数直接来自 `url_table()` 的三条记录：users、scores、slow。这个计数不看 fetch 是否成功，只反映输入规模，所以即使 slow 被取消，总 URL 数仍然是 3。

- 成功 2。

  **答案解析：** watchdog 在 200ms 后请求 stop，而前两条 URL 的模拟延迟是 100ms 和 150ms。它们在 stop 请求前完成，`fetch_one` 返回 `Ok` 并保留 body。aggregate 按 `Ok` 状态计入成功，所以成功数是 2。

- stopped 1。

  **答案解析：** 第三条 slow URL 延迟 300ms，等待期间会遇到 200ms watchdog 发出的 stop 请求。`timer_awaiter` 观察 token 后恢复协程，`fetch_one` 在 `co_await` 后第二次检查 token，返回 `Stopped`。aggregate 因此把它计入 stopped。

- 解析成功行数 4。

  **答案解析：** parser 只处理成功 fetch 的 body。前两条成功响应各有 header 加两行有效数据：Alice、Bob、Carol、Dave。header 被跳过，四条有效数据行 yield `ok=true`，所以解析成功行数是 4；slow URL 的 body 因 stopped 不会进入 parser。

- 总分 343。

  **答案解析：** 总分只累加 `ok=true` 的记录分数。前两条成功 body 的分数是 `85 + 92 + 78 + 88`，结果为 343。第三条 slow body 中的 Eve/Frank 没被解析，因为该 fetch 走 stopped 路径。

- 整体耗时低于串行总和。

  **答案解析：** 串行抓取三条 URL 约为 `100ms + 150ms + 300ms = 550ms`。并发版通过 scope 同时启动多个 fetch，前两条先完成，第三条在 200ms 左右被 stop 收束；整体耗时接近超时阈值加少量调度开销。低于串行总和说明 `when_all_fetch` 不是逐条等待。

完成后，能不看代码讲清：stop token 如何从 main 传到 fetch，scope 在哪里等齐，parser generator 何时推进，超时 URL 为什么不覆盖成功 URL 的报告。

**答案解析：** main 创建 stop source 和 watchdog，把 token 传给 aggregate，aggregate 传给 when_all_fetch，再传到每个 fetch_one。when_all_fetch 用 scope 启动所有 fetch_into，并在 `scope.join()` 等齐；aggregate 之后只遍历 `Ok` body，逐行推进 `parse_lines(std::string body)`。超时只让未完成的 slow 返回 `Stopped`，已完成 fetch 的结果槽已经写好并按状态被 aggregate 保留。

## Student 检查

`main.cpp` 现在先检查 parser，再检查整条爬虫流水线：

| Part | 操作 | 本地检查 |
| --- | --- | --- |
| Part 1 | 调用图和 owner | 仍是文字作业，答案解析给出完整拓扑 |
| Part 2 | `fetch_one` 两次 stop 检查 | 200ms watchdog 后 slow URL 必须为 `Stopped` |
| Part 3 | `parse_lines(std::string body)` | 跳过 header，解析 `Alice,85`/`Bob,92`，坏行 `ok=false` 后继续 |
| Part 4 | `when_all_fetch` 并发+scope | 总耗时必须低于 350ms，证明不是 550ms 串行抓取 |
| Part 5 | `aggregate` 汇总 | total=3、ok=2、stopped=1、err=0、lines=4、score=343 |

完成前：当前 parser 分数占位、串行抓取或漏 stop 检查都会被本地检查拒绝
