# 练习 6：有界记录流水线

## 目标

构造一次性同步流水线：输入记录被分成三条逻辑分支，每条分支处理一批记录，阶段为 `parse -> enrich`，最后在 merge scheduler 上收束成 `Report`。

## Part 1：输入边界

`run_pipeline(records, max_records)` 支持 0 到 `max_records` 条记录。超过上限立即抛 `std::invalid_argument`，表示本函数拒绝新 work。

记录语法为 `name,nonnegative_decimal`：`name` 以字母或 `_` 开头，后续允许字母、数字、`_`；数值只能是十进制非负整数，范围 `0..1'000'000`。解析溢出、负数、超出计算边界、多逗号或格式错误都记为 invalid record，不进入 enrich。

## Part 2：三条逻辑分支

输入按 round-robin 分进三批。即使输入少于三条，也仍构造三条 branch；空 branch 也要完成。每个 branch 复制自己要处理的字符串，因此跨 scheduler 边界不持有调用者局部 `string_view`、`span` 或引用。

## Part 3：真实 enrich 计算

`run_pipeline(records, max_records, EnrichHook)` 允许 checker 注入 score 函数。实现必须在 enrich 阶段调用该 hook，并使用 hook 返回值进入 merge。默认 overload 只提供普通演示 score；checker 使用自己的 hook 记录实际 thread id 和调用次数，防止把 score 预先写死在 parse 阶段。

## Part 4：执行资源证据

`Report` 记录：

- caller thread；
- 每个 parse/enrich stage 的 thread id；
- merge thread；
- `completed_batches == 3`。

checker 要求 parse 与 enrich 的 thread id 集合互不重叠，且 enrich hook 的 thread id 出现在 recorded enrich stages 中。单 scheduler 假完成体会编译通过，但作为 bad 回归必须 exit 1。

## Part 5：收束边界

本题是同步一次性函数，`sync_wait` 返回就是收束边界：三个已接受 batch 都完成，merge 已生成最终 report，局部 buckets 和记录副本可安全销毁。普通解析失败不会展开异常，而是计入 invalid；超限输入在提交任何 branch 前拒绝。

本题不实现通用 close/reject 对象，不声称覆盖 shutdown 后拒绝新提交或已接受 work drain 的完整运行时协议。那些由 H1 run_loop 和 P1 pipeline 项目主讲；本题只给它们准备“有界输入、已接受工作必须在返回前收束”的前置模型。
