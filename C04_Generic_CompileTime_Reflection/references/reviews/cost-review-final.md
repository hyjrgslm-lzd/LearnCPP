# C04 B01 compile cost 最终非作者审查

日期：2026-09-10。

结论：`ITERATE`。正式数据本身复算通过，但当前正式结果和正文缺少本机计时分辨率边界；补齐后可再终审。未发现数据丢失、source SHA 绑定错误、section/trace parser 错误或样本门错误。

## 文件与数据范围

- Driver：`C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py`。
- 正文：`C04_Generic_CompileTime_Reflection/chapters/12-compile-cost.md`。
- 练习说明：`C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost/README.md`。
- 正式结果：`C04_Generic_CompileTime_Reflection/references/benchmarks/results/compile-cost-run-20260910-003243/raw.json`、`events.jsonl`、`summary.md`。
- 当前入口：`C04_Generic_CompileTime_Reflection/references/benchmarks/results/latest-summary.md`。
- 作者外层记录：`C04_Generic_CompileTime_Reflection/references/validation/cost-author/09-full-driver.json`、`10-full-driver-after-warmup-gate.json`、`11-full-driver-after-summary-boundary.json`。

## 已复算通过

- `raw.json` 为 `status=PASS`、`mode=full`；`repo_head=f261bea559d6722c31135fb2d3589be52fe958ed`。
- 当前 `cost_driver.py` SHA-256 为 `ed2b27e543e314345d3fe74d6b914f7d2c5c11969b2437614759b33bf98db699`，与 `raw.json.tools.driver_sha256` 相同。
- `summary.md` 与 `latest-summary.md` 内容一致。
- `events.jsonl` 共 534 条，最后一条为 `finish/PASS`，指向最终 `raw.json`、`summary.md`、`latest-summary.md`。
- sample event 共 48 条，等于 raw 中 36 条 type query 样本加 12 条 multi TU 样本。
- 8 组样本门均为 1 个成功 warmup 和 5 个 valid sample；没有 phase=sample 的 invalid 样本混入中位数。
- 所有 valid sample 都满足 compile PASS、`idle_before.idle=true`、`post_probe.idle=true`、`pre_to_post_overlap.idle=true`。
- type query 中位数与范围独立复算一致：32 recursive/fold 为 0.0940/0.0940，128 为 0.1090/0.1100，256 为 0.1410/0.1090。
- multi TU 中位数与范围独立复算一致：implicit 0.3590，范围 0.3280-0.4530；explicit 0.4060，范围 0.3900-0.4370。
- generated source SHA 全部匹配 raw；timing sample object artifact SHA 全部匹配现存产物。
- 从 stored `llvm-readobj --sections` stdout 独立复算 `.text` section：implicit 调用方 `.obj` raw 336、全部 `.obj` raw 705、exe virtual 90861；explicit 为 28、474、90861。
- 从实际 trace JSON 独立复算 `Total ...` totals，与 raw 一致。`Total InstantiateClass` count 为 recursive 6、fold 3，正文已按粗粒度线索处理，没有当作嵌套实例化总数。
- 09 full 失败 raw 与 recorder 保留；10 full PASS 与 11 final PASS 均保留，最终正文和 latest 指向 11 的 `compile-cost-run-20260910-003243`。

## 阻断问题

[MEDIUM] 正式结果缺少本机 `time.monotonic` 分辨率边界

位置：

- `C01_Build_Compile_Link/exercises/tools/process_runner.py:23` 与 `:73` 使用 `time.monotonic()` 计算 `process_seconds`。
- `C04_Generic_CompileTime_Reflection/references/benchmarks/results/latest-summary.md:5` 到 `:14` 的环境段没有记录 clock implementation/resolution。
- `C04_Generic_CompileTime_Reflection/references/benchmarks/results/latest-summary.md:29` 到 `:56` 报告到 0.001s 的样本值。
- `C04_Generic_CompileTime_Reflection/chapters/12-compile-cost.md:44` 与 `:64` 引用中位数差异，但没有说明本机 clock 分辨率。
- `C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost/README.md:17` 汇总结论，也没有提示不要从毫秒级差异推断收益。

问题：本机 `time.get_clock_info("monotonic")` 返回 `implementation=GetTickCount64()`、`resolution=0.015625`。正式样本呈 15.6ms 左右阶梯，当前结果虽没有把 32/128 的 1ms 差异说成收益，但正式 summary/正文没有显式记录这个分辨率边界。读者容易把 0.1090 vs 0.1100 这种展示精度误读成有效毫秒级差异。

修复：把 monotonic clock 信息写入 driver 的 environment record 和生成 summary；在 chapter12/B01 README 的结果边界补一句：本机 `time.monotonic` 分辨率为 0.015625s，不能从低于或接近该分辨率的毫秒级差异推断收益。因为这会改变 driver SHA 与正式 raw 绑定，建议用更新后的 driver 重新跑一次 full，生成新的 `compile-cost-run-*`、`latest-summary.md`，再更新 chapter12/B01 README 指向新目录。

## 非阻断边界

- `Total InstantiateClass count` 不随 N 增长已被正文限定为粗粒度 `Total ...` 线索；归因主要应看 `Total Frontend` 与正式 timing，而不是把 count 当嵌套实例化次数。
- 当前 driver 的 warmup 门只要求 warmup 编译 PASS 且前置 idle；warmup 不进入有效样本，后续每个正式 sample 仍单独 `wait_for_idle()` 并做 post/overlap 检查。这个修复没有掩盖正式样本污染。
- 本轮未重新跑 full benchmark，未启动 compiler。

## 建议

修复上述 clock boundary 后再交终审；其余数据链路不需要重复大排查，只需复算新 raw 的 source SHA、样本门、中位数、section、trace 和 events。
