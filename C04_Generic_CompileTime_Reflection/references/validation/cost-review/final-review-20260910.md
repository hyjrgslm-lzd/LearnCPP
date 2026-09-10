# cost-review final 独立复验证据

日期：2026-09-10。

## 命令与结果

- `python -B -m py_compile C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py`：PASS。
- problematic pattern scan：未命中 `console.log`、空 catch、硬编码 `apiKey`、裸 `except:`、`best effort`、`silent`、`fallback`。
- `git diff --stat -- C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py C04_Generic_CompileTime_Reflection/chapters/12-compile-cost.md C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost/README.md C04_Generic_CompileTime_Reflection/references/benchmarks/results/latest-summary.md`：无输出，当前审查对象相对 index 无 diff。
- `time.get_clock_info("monotonic")`：`implementation=GetTickCount64()`、`resolution=0.015625`、`monotonic=True`、`adjustable=False`。

## raw 复算

- raw path：`C04_Generic_CompileTime_Reflection/references/benchmarks/results/compile-cost-run-20260910-003243/raw.json`。
- status/mode：`PASS` / `full`。
- raw driver SHA 与当前 `cost_driver.py` SHA：`ed2b27e543e314345d3fe74d6b914f7d2c5c11969b2437614759b33bf98db699`。
- events：534 条；`sample` 48 条；最终 `finish.status=PASS`。
- sample gates：6 个 type query 组 + 2 个 multi TU 组，全部 `warmup_success_count=1`、`valid_sample_count=5`、`status=PASS`。
- valid sample window：全部满足 compile PASS、pre idle、post idle、pre-to-post overlap idle。
- source/artifact SHA：generated source 全部匹配；timing sample object artifact 全部匹配。
- section parser：从 stored `llvm-readobj --sections` stdout 独立复算，错误 0。
- trace parser：从实际 trace JSON 独立复算 `Total ...` map，错误 0。

## 复算统计

| group | kind | count | median | min | max |
|---|---|---:|---:|---:|---:|
| type_query | recursive | 32 | 0.0940 | 0.0940 | 0.1100 |
| type_query | fold | 32 | 0.0940 | 0.0780 | 0.1090 |
| type_query | recursive | 128 | 0.1090 | 0.1090 | 0.1250 |
| type_query | fold | 128 | 0.1100 | 0.0940 | 0.1560 |
| type_query | recursive | 256 | 0.1410 | 0.1410 | 0.1560 |
| type_query | fold | 256 | 0.1090 | 0.0930 | 0.1090 |
| multi_tu_compile | implicit |  | 0.3590 | 0.3280 | 0.4530 |
| multi_tu_compile | explicit |  | 0.4060 | 0.3900 | 0.4370 |

## section headline

| kind | caller obj text raw | all obj text raw | exe text virtual |
|---|---:|---:|---:|
| implicit | 336 | 705 | 90861 |
| explicit | 28 | 474 | 90861 |

## trace headline

| kind | Total Frontend us | Total InstantiateClass count |
|---|---:|---:|
| recursive_32 | 21228 | 6 |
| recursive_128 | 33602 | 6 |
| recursive_256 | 78501 | 6 |
| fold_32 | 18940 | 3 |
| fold_128 | 22194 | 3 |
| fold_256 | 23314 | 3 |
