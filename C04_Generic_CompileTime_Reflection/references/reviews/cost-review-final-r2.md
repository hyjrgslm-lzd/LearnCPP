# C04 B01 compile cost 最终非作者审查 r2

日期：2026-09-10。

结论：`APPROVE`。批准当前版本与 `references/benchmarks/results/compile-cost-run-20260910-004959/` 的有限结论。批准范围只限本机 Windows / clang-cl 22.1.3 / C01 runner `time.monotonic()` 协议；不推广到其他编译器、平台、构建系统或更大项目。

## 审查范围

- Driver：`C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py`。
- 正文：`C04_Generic_CompileTime_Reflection/chapters/12-compile-cost.md`。
- 练习说明：`C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost/README.md`。
- 正式结果：`C04_Generic_CompileTime_Reflection/references/benchmarks/results/compile-cost-run-20260910-004959/raw.json`、`events.jsonl`、`summary.md`。
- 当前入口：`C04_Generic_CompileTime_Reflection/references/benchmarks/results/latest-summary.md`。
- 作者外层记录：`C04_Generic_CompileTime_Reflection/references/validation/cost-author/12-full-driver-after-clock-boundary.json`。

## 复算结论

- `raw.json` 为 `status=PASS`、`mode=full`。
- 当前 `cost_driver.py` SHA-256 为 `b4fb3e1cd2b62d037593a184141d6279538df7bb6dd572e5231a5875651a0eb9`，与 `raw.json.tools.driver_sha256` 相同。
- `summary.md` 与 `latest-summary.md` 内容一致。
- `events.jsonl` 共 534 条；sample event 48 条；最后一条为 `finish/PASS`。
- 8 组样本门均为 1 个成功 warmup 和 5 个 valid sample；所有 valid sample 都满足 compile PASS、pre idle、post idle、pre-to-post overlap idle。
- raw 环境记录包含 clock 元信息：`time.monotonic()` 为 `GetTickCount64()`、resolution `0.015625`；`time.perf_counter()` 为 `QueryPerformanceCounter()`、resolution `1e-07`。driver 仍保持 C01 runner 的 `monotonic` 计时协议。
- summary、chapter12 和 B01 README 都明确 4 位小数用于复算展示，不能从 1ms 级差异推断收益；原 clock 阻断关闭。
- 旧 run 保留：`compile-cost-run-20260910-002405` 为 full FAIL，`002853`、`003243`、`004959` 为 full PASS；当前 latest 指向 `004959`。

## 批准的有限结论

- 32 项 type query：recursive/fold 中位数同为 0.0940s；本轮不能分辨稳定收益。
- 128 项 type query：recursive 0.1100s，fold 0.1090s；差异低于 clock 分辨率解释边界，本轮不能推断 fold 有收益。
- 256 项 type query：recursive 0.1570s，fold 0.1090s；结合 `Total Frontend` trace，可说本机该输入下 fold 减少前端工作并带来可见编译时间收益。
- `Total InstantiateClass count` 只作为 clang trace 的 `Total ...` scope count，不作为所有嵌套实例化总数。
- multi TU：explicit 减少调用方 `.obj` `.text` raw 合计 336 -> 28，全部 `.obj` `.text` raw 合计 705 -> 474；最终 `.exe` `.text` virtual 两者同为 90861。
- multi TU timing：implicit 中位数 0.3750s，explicit 中位数 0.4380s；本机该口径没有证明 explicit 更快，且该值为多个短编译进程耗时相加，解释需谨慎。

## 问题

无阻断问题。

## 验证

- `python -B -m py_compile C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py`：PASS。
- problematic pattern scan：未命中空 catch、硬编码 `apiKey`、`best effort`、`silent`、`fallback`。
- 独立 Python 复算：source SHA、样本门、events、clock 元信息、summary/latest 绑定、section headline 均 PASS。
- 未运行 full benchmark，未启动 compiler。
