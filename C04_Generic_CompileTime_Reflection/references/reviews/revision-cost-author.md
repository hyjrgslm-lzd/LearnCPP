# C04 B01 meta-map cost author review

日期：2026-09-10。

范围：

- `references/benchmarks/cost_driver.py`
- `references/benchmarks/meta_lookup_cost.py`
- `chapters/12-compile-cost.md`
- `exercises/B01_compile_cost/README.md`
- `references/validation/revision-20260910/cost-meta-map-checks/meta-map-run-20260910-124819/`

结论：PASS，限于 check-only。正式 benchmark 未运行，等待独占测量窗口。

检查项：

| 项 | 结果 | 证据 |
|---|---|---|
| Mp11 pin | PASS | marker commit 和 Git HEAD 均为 `b94b089d4ec83cd397f20958f34edf25bc3e06f4`，依赖工作树干净 |
| 生成组数 | PASS | 32/128/256 * last/missing * manual/mp11 = 12 个 TU |
| 同输入输出 | PASS | manual 与 Mp11 源码使用同一 `source_map`、同一 `entry_value` 适配、同一 Boost.Mp11 头依赖；只切换 `manual_find` / `mp_map_find` |
| 正例控制 | PASS | 12 个 TU 均 baseline compile/link/run 通过 |
| 缺键语义 | PASS | generated source 对 missing 使用 `answer == void`，程序返回 `-1` |
| trace 分离 | PASS | 12 个 trace compile 通过，summary 只列定位数据 |
| timing 边界 | PASS | check-only 样本数为 0；无正式时间结论 |
| 旧 B01 行为 | PASS | `--meta-map` 独立开关；非 meta-map 正式运行仍只更新旧 `latest-summary.md` |

本轮验证：

```powershell
python -m py_compile C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py C04_Generic_CompileTime_Reflection/references/benchmarks/meta_lookup_cost.py
python C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py --output-root C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/cost-meta-map-checks --meta-map --check-only
python -c "import json, pathlib; p=pathlib.Path('C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/cost-meta-map-checks/meta-map-run-20260910-124819/raw.json'); d=json.loads(p.read_text(encoding='utf-8')); mm=d['meta_map']; print(d['status']); print(mm['dependency']['status'], mm['dependency']['marker_commit'], mm['dependency']['actual_commit'], mm['dependency']['worktree_clean']); print(len(mm['sources']), len(mm['correctness']), len(mm['trace']), len(mm['samples'])); print(sum(1 for v in mm['correctness'].values() if v['compile']['status']=='PASS' and v['link']['status']=='PASS' and v['run']['status']=='PASS')); print(sum(1 for v in mm['trace'].values() if v['compile']['status']=='PASS'))"
```

验证输出要点：

```text
CHECK_ONLY_PASS
PASS b94b089d4ec83cd397f20958f34edf25bc3e06f4 b94b089d4ec83cd397f20958f34edf25bc3e06f4 True
12 12 12 0
12
12
```

限制：

- `Get-CimInstance Win32_Processor` 在当前受限环境返回权限失败；driver 已保留该失败并使用环境 fallback，不影响主体 compile/link/run 判定。
- check-only 期间侦测到其它 `cmake/MSBuild` 活动；该模式不产生正式 timing 结论。正式 `--meta-map` 需要独占窗口再运行。
