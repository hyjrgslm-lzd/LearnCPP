# Capstone 1：异步小爬虫

## 目标

把模块 A/B/C 的能力拼成一个完整项目：
并发抓取 N 个 URL -> generator 解析 -> when_all 汇合 -> stop_token 超时 -> async_scope 收束。

## 文件结构

- `url_table.hpp`：3 条模拟 URL 记录（内容 + 延迟）。
- `parser.hpp`：`std::generator<Record>` 按行惰性 yield。
- `main.cpp`：fetch / aggregate / 主流程 / 报告输出。

## 必做任务

1. fetch 协程：接收 URL + stop_token，模拟延迟，被取消则走 stopped 路径。
2. parse generator：按行 `co_yield`，跳过空行，错误行 yield 失败标记继续 yield。
3. aggregate 协程：用 when_all 汇合 fetch 结果，遍历 generator 计行数。
4. 整体超时：watchdog 协程在 N ms 后 `request_stop()`。
5. async_scope：所有并发 fetch 协程在 scope 内启动，scope 析构等齐。
6. 输出报告：成功 / 超时 / 失败 / 总行数 / 整体耗时 / 每 URL 明细。
7. 在文件顶部注释中补全协程调用图。

## 设计约束

- 至少 3 类异步阶段（fetch / parse / aggregate）。
- 至少 1 次 stop_token 检查。
- 至少 1 次 when_all 或 when_any。
- 禁止 detach、裸 std::thread、全局可变状态。

## 验收点

- 协程调用图能指出所有并发点、合流点、取消传播路径、scope 收束边界。
- 超时的 URL 走 stopped 路径，不让整个报告失败。
- 所有协程帧的拥有者明确（scope 或调用者）。
- generator 正确惰性产出，不一次性加载全部行。
- 你能不看代码描述整个项目的协程拓扑。

## Starter / Reference

- `main.cpp` 是结课骨架，保留 TODO，串起 fetch / parse / aggregate / timeout。
- `solution.cpp` 是可运行参考实现，`ctest --preset verify-core -C Release -R Capstone1_async_crawler_reference`
  会校验本地模拟 crawler 并发抓取、超时取消、逐行解析、聚合统计和整体收束。
