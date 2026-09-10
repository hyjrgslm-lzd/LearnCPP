# C04 B01 compile cost 非作者审查

日期：2026-09-10。

结论：`APPROVE_READY`。本结论只批准进入正式采样，不批准正式性能结果；正式样本尚未采集，后续仍需用 full 模式生成的 `references/benchmarks/results/latest-summary.md` 和同级 `raw.json` 终审。

## 审查范围

- `CONTENT_REFACTORING_GUIDE.md` 第 6、11、13 节。
- `C04_Generic_CompileTime_Reflection/references/implementation-spec.md`。
- `C04_Generic_CompileTime_Reflection/chapters/12-compile-cost.md`。
- `C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost/**`。
- `C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py`。
- 作者 CHECK_ONLY 证据：`C04_Generic_CompileTime_Reflection/references/validation/cost-author/driver-checks/compile-cost-run-20260910-001031/raw.json`、`summary.md`、trace JSON、stored tool stdout。

## 核对结果

- 规格要求的两组实验已在 driver 中固定：32/128/256 的 recursive/fold type query，以及 4TU implicit vs 4TU+provider explicit；正文也明确 trace 只作定位、正式 timing 单独采样。
- type query 生成源码覆盖 `found`、`missing`、`empty` 三个边界，并由 `static_assert` 和可执行返回值绑定。
- multi TU 生成源码使用 `std::uint32_t` 模 2^32 运算；`main` 对每个 `use_N` 做独立 reference 核对，并要求 implicit/explicit stdout 完全相同。
- COFF section parser 解析 stored `llvm-readobj --sections` 输出，区分 `.obj` 与 `.exe`；独立复算 `.text` section count/raw/virtual 与 `raw.json` 一致。
- driver 在 correctness、trace、section parser 失败时直接 `RuntimeError`，并在失败路径写 `raw.json` 与 `events.jsonl`；没有发现吞错后继续计时的 fallback。
- full 模式强制每个组 1 个成功 warmup + 5 个 valid sample；CHECK_ONLY 模式状态为 `CHECK_ONLY_PASS`，事件日志 `finish.latest_summary=null`，不发布正式 latest summary。
- 干扰检测按 PID/CPU 比较，驻留 `MSBuild.exe` 可放行，busy/new/exited/detection failure/4 samples 控制均 PASS；章节已说明短任务仍可能漏检。
- `references/benchmarks/results` 目前没有 `latest-summary.md`。旧空 run 目录无 `raw.json`，不能构成正式结果。

## 验证

- `python -B -m py_compile C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py`：PASS。
- problematic pattern scan：未命中 `console.log`、空 catch、硬编码 `apiKey`、裸 `except:`、silent/best-effort fallback 等审查模式。
- 独立 Python 复验：`independent_parser_errors 0`；trace totals 与 raw 记录一致；section 汇总与 stored tool stdout 一致。
- `compile-cost-run-20260910-001031/raw.json`：`status=CHECK_ONLY_PASS`、`mode=check-only`、`type_samples=0`、`multi_samples=0`、detector self-check PASS、implicit/explicit stdout 相等。

## 问题

无阻断问题。

## 边界

- 本轮未运行正式 full benchmark，未批准任何性能中位数、范围、加速结论或最终 chapter 数字。
- 当前环境没有可用 `lsp_diagnostics` 工具；对 Python driver 使用 `py_compile` 和独立 JSON/tool-output parser 复验替代。
