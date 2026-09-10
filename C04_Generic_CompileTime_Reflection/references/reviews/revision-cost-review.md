# C04 meta-map 成本实验准备审查

## 结论

Verdict: COMMENT

本轮 `meta-map` 扩展的 check-only 与正式实验准备未发现实质门控缺口。当前证据只支持“正确性、trace 定位、依赖 pin、driver 门控准备通过”，不支持任何正式 timing 收益结论。正式 raw 出来后还需要复算 1 次预热/5 个有效样本、中位数、噪声与重叠门。

## 范围

- `C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py`
- `C04_Generic_CompileTime_Reflection/references/benchmarks/meta_lookup_cost.py`
- `C04_Generic_CompileTime_Reflection/chapters/12-compile-cost.md`
- `C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost/README.md`
- check-only evidence: `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/cost-meta-map-checks/meta-map-run-20260910-124819`

## Stage 1 - Spec Compliance

PASS.

- 同一输入契约：`meta_lookup_cost.py:56-64` 生成同一个 `source_map`，`meta_lookup_cost.py:72-76` 生成同一个 `entry_value`，`meta_lookup_cost.py:90-93` 只切换 `manual_find` 与 `boost::mp11::mp_map_find`。
- 正确性先于正式计时：`cost_driver.py:623-649` 对每个 variant 先 compile/link/run，并检查 manual/mp11 stdout 相等；`cost_driver.py:630-645` 记录 trace；`cost_driver.py:652-680` 只有 `include_timing` 为真才进入正式采样。
- 样本门：`cost_driver.py:657-663` 为每个 kind 安排 1 次 warmup + 5 次 sample；`cost_driver.py:716-724` 要求每组 1 个成功 warmup 和 5 个 valid sample；`cost_driver.py:724-726` 在正式模式失败即抛错。
- 空闲/重叠门：`cost_driver.py:666-672` 每个样本前 wait idle，样本后 probe，并要求 `idle_before`、`post_probe`、`pre_to_post_overlap` 都为真。
- 计时边界：`cost_driver.py:66-72` 用 `perf_counter` 包住 runner；`cost_driver.py:677` 在 compile 返回后才读取 artifact hash；`cost_driver.py:787-789` 摘要明确 hash 不进计时、trace 不混入正式计时。
- 旧 B01 latest 兼容：`cost_driver.py:1021-1024` 只在非 check-only 且非 meta-map 的原实验路径更新 `latest-summary.md`；`B01_compile_cost/README.md:33` 明确 `--meta-map` 不覆盖旧 latest。
- 依赖 pin：`cost_driver.py:568-601` 同时检查 marker commit、实际 HEAD、dependency worktree clean；check-only raw 的 `raw.json:2050-2105` 记录 expected/actual commit 相同且 clean。
- 文档边界：`chapters/12-compile-cost.md:108-122` 明确 12 组、同输入/同输出适配、正确性/trace/timing 分离、不可简化成“库一定更快”；`B01_compile_cost/README.md:19-33` 同步依赖 pin、check-only、正式命令和 latest 边界。

## Root-Cause Fallback Guard

PASS.

未发现用 fallback/workaround 掩盖失败的路径。`environment_record()` 的 CPU fallback 保留了原始 CIM 失败记录并只补充环境元数据，不参与通过/失败判定；`read_dependency_marker()` marker 缺失时返回空字典，但 `verify_mp11_dependency()` 会因 marker/commit 条件失败而拒绝。

## Stage 2 - Code Quality / Static Checks

PASS with validation gap noted.

- Python AST parse: PASS, 2 files.
- Pattern scan: PASS, 未命中 `console.log`、空 catch、硬编码 key/token/password、`except: pass`、`best effort`、静默 `{}` fallback。
- 独立生成物复算：PASS。对 `manual_*/mp11_*` 6 对 generated TU 比较，均只有 1 行差异，且差异行为是 `manual_find` vs `mp_map_find`。
- Check-only raw 复算：PASS。`mode=meta-map-check-only`，`status=CHECK_ONLY_PASS`，`samples=0`，`sources=12`，`trace=12`，`correctness=12`，`detector=PASS`，`initial_probe_idle=False`。它没有冒充正式采样。
- `lsp_diagnostics`/`ast_grep_search` 工具在当前可用工具面中不可调用；已用 AST parse、`rg` 静态模式和 JSON 复算替代。

## Issues

Total Issues: 0

## Recommendation

COMMENT.

当前代码和文档可进入 root 安排的正式采样窗口。正式 raw 回来后，必须再做一次独立复算：每个 `(count,target,kind)` 必须恰好 1 个成功 warmup、5 个 valid sample；所有 valid sample 必须无 timeout、无 active build overlap；只能从正式 `perf_counter_seconds` 和 trace 定位共同解释结果，不能从 check-only 或零样本声明收益。
