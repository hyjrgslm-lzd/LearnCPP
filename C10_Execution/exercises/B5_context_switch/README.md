# B5 context switch

## 目标

用三个小图观察 `starts_on`、`continues_on` 和 `on`。重点不是线程池 benchmark，而是把执行位置写进 sender 图，并用可观察线程 ID 验证边界。

## Part 1：真实解析

输入格式是 `name,value,raw`，例如 `sensor_b,7,12.5`。`parse_record` 必须解析传入字符串，不能返回固定 `sensor_a,42,3.14`。checker 使用多组 fresh input，并按 `normalized = raw / 100`、`score = value * normalized` 验证。

## Part 2：`starts_on`

`starts_on(parse_sch, sender)` 表示这段 sender 在 `parse_sch` 上开始。练习先在 parse pool 里解析，再用 `let_value` 返回一段 `starts_on(compute_sch, ...)`，让 compute 阶段整体在 compute pool 上开始。

## Part 3：`continues_on`

第二条图先在 parse pool 解析，然后 `continues_on(compute_sch)`，后面的 compute `then` 在 compute pool 上继续。checker 要求两条图数值一致，并要求 compute 离开 caller thread。

## Part 4：`on` roundtrip

固定 stdexec `nvhpc-26.05` 的 `on(scheduler, sender)` 在有外层 scheduler 环境时，child 在目标 scheduler 上运行，completion 回到旧 scheduler。练习用 outer pool 启动外层图，再用 compute pool 运行 inner work，最后 continuation 回到 outer pool。

## 边界

`distinct_pools_observed` 必须从实际 parse/compute thread id 推导；不能写常量 true。线程 ID 是观察证据，不是 scheduler 身份本身。
