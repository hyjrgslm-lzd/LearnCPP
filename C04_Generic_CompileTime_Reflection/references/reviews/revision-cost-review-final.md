# C04 meta-map 正式成本实验最终审查

## Code Review Summary

**Verdict:** APPROVE

**Files/data reviewed:** 7

**Total issues:** 0

### By Severity

- CRITICAL: 0
- HIGH: 0
- MEDIUM: 0
- LOW: 0

## Scope

- `references/benchmarks/results/meta-map-run-20260910-143607/raw.json`
- `references/benchmarks/results/meta-map-run-20260910-143607/summary.md`
- `references/benchmarks/results/meta-map-run-20260910-143607/events.jsonl`
- `references/validation/revision-20260910/root-meta-map-formal-01.json`
- `references/validation/revision-20260910/cost-independent-final.json`
- `references/benchmarks/cost_driver.py`
- `references/benchmarks/meta_lookup_cost.py`

## Stage 1 - Spec Compliance

PASS.

- 外层记录器 PASS：`root-meta-map-formal-01.json` 记录 `exit_code=0`、`timeout=false`、`verdict=PASS`，stdout 指向 `meta-map-run-20260910-143607`，`latest_summary=null`。
- driver formal PASS：`raw.json` 为 `mode=meta-map`、`status=PASS`、`repo_head=f261bea559d6722c31135fb2d3589be52fe958ed`。
- 12 组样本门独立复算 PASS：每个 `(count,target,kind)` 均为 1 个 quiet warmup 和 5 个 valid sample。quiet warmup 检查了 `compile.status=PASS`、无 timeout、`idle_before.idle=true`、`post_probe.idle=true`、`pre_to_post_overlap.idle=true`。
- 72 条 sample 记录齐全：`events.jsonl` 复算得到 `sample=72`、`source=12`、`section_parse=24`、`phase_complete=6`、`finish=1`。
- 正确性先于 trace 再进入 timing：`cost_driver.py:623-649` 先 compile/link/run 并检查 manual/mp11 输出；`cost_driver.py:630-645` 单独 trace；`cost_driver.py:652-680` 之后才进入 timing。
- timing 没混 trace：72 条 warmup/sample timing compile command 均无 `ftime-trace` 参数；12 条 trace compile command 均有 trace 参数并有 totals。
- 同源同契约：`meta_lookup_cost.py:56-64` 生成同一 `source_map`，`meta_lookup_cost.py:72-76` 生成同一 `entry_value`，`meta_lookup_cost.py:90-93` 只切换 `manual_find` 与 `boost::mp11::mp_map_find`。generated source 独立比较确认 6 对 manual/mp11 都只差 1 行查找算法。
- 依赖 pin 通过：Mp11 marker/actual HEAD 均为 `b94b089d4ec83cd397f20958f34edf25bc3e06f4`，dependency worktree clean；`cost_driver.py:568-601` 会在 marker、actual commit 或 dirty 不匹配时失败。
- 原 B01 latest 未覆盖：`cost_driver.py:1021-1024` 只在非 meta-map 路径更新 `latest-summary.md`；本次 outer stdout 和 raw 均为 `latest_summary=null`。

## Independent Recalculation

独立结果保存于 `references/validation/revision-20260910/cost-independent-final.json`，verdict 为 PASS。

| 项数 | 查询 | manual median | manual range | mp11 median | mp11 range | 中位数较低 | 范围重叠 |
|---:|---|---:|---|---:|---|---|---|
| 32 | last | 0.1385 | 0.1303-0.1461 | 0.1441 | 0.1271-0.1685 | manual | yes |
| 32 | missing | 0.1467 | 0.1356-0.1561 | 0.1398 | 0.1348-0.1422 | mp11 | yes |
| 128 | last | 0.1568 | 0.1438-0.1716 | 0.1382 | 0.1329-0.2246 | mp11 | yes |
| 128 | missing | 0.1476 | 0.1425-0.1702 | 0.1447 | 0.1357-0.2128 | mp11 | yes |
| 256 | last | 0.1606 | 0.1514-0.1901 | 0.1448 | 0.1240-0.1722 | mp11 | yes |
| 256 | missing | 0.1596 | 0.1352-0.1777 | 0.1286 | 0.1264-0.1498 | mp11 | yes |

## Observations Allowed

- 可以说：在本机这次 5-sample formal run 中，6 个 manual-vs-Mp11 对比里有 5 个 Mp11 样本中位数较低。
- 可以说：trace 定位中，128/256 的 Mp11 `Total EvaluateAsConstantExpr count` 明显低于 manual；256 组 `Total Frontend us` 也低于 manual。
- 可以说：本实验隔离的是同一 Boost.Mp11 `mp_list` type-map 输入下的查找机制形状，计时口径包含 Python runner、进程创建、clang-cl 编译和等待返回。
- 不能说：Mp11 普遍或稳定更快。所有 6 对 perf_counter 范围都重叠，且样本数只有 5。
- 不能说：这个结果推广到所有 schema/type-map、所有编译器、所有库版本或所有机器。
- 不能用 trace 单独替代正式计时；trace 是定位证据，不是正式 timing 样本。

## Stage 2 - Quality / Security

PASS.

- 未发现 fallback/workaround 掩盖失败。CPU metadata fallback 只补环境信息，原始失败仍保存在 raw；依赖 marker 缺失不会被 fallback 洗成 PASS。
- 未发现硬编码凭据、空 catch、静默 best-effort 成功路径。
- 本轮未运行编译或 benchmark，符合复算任务约束。

## Recommendation

APPROVE 本次 meta-map 正式实验证据与文档边界。批准范围限于本机正式 run 的可复算观察；不批准任何跨平台、跨编译器、跨项目规模的稳定加速声明。
