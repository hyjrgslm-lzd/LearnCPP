# cost-review final r2 独立复验证据

日期：2026-09-10。

## raw 绑定

- raw path：`C04_Generic_CompileTime_Reflection/references/benchmarks/results/compile-cost-run-20260910-004959/raw.json`。
- status/mode：`PASS` / `full`。
- driver SHA：`b4fb3e1cd2b62d037593a184141d6279538df7bb6dd572e5231a5875651a0eb9`。
- current source SHA：`b4fb3e1cd2b62d037593a184141d6279538df7bb6dd572e5231a5875651a0eb9`。
- `summary.md == latest-summary.md`：true。
- events：534 条；sample events：48 条；final event：`finish/PASS`。

## clock 元信息

- `time.monotonic()`：implementation `GetTickCount64()`，resolution `0.015625`，monotonic true，adjustable false。
- `time.perf_counter()`：implementation `QueryPerformanceCounter()`，resolution `1e-07`，monotonic true，adjustable false。
- summary/chapter12/B01 README 已记录 4 位小数不等于 1ms 精度，不从 1ms 级差异推断收益。

## 样本门

| group | kind | count | warmup | valid samples | status |
|---|---|---:|---:|---:|---|
| type_query | recursive | 32 | 1 | 5 | PASS |
| type_query | fold | 32 | 1 | 5 | PASS |
| type_query | recursive | 128 | 1 | 5 | PASS |
| type_query | fold | 128 | 1 | 5 | PASS |
| type_query | recursive | 256 | 1 | 5 | PASS |
| type_query | fold | 256 | 1 | 5 | PASS |
| multi_tu_compile | implicit |  | 1 | 5 | PASS |
| multi_tu_compile | explicit |  | 1 | 5 | PASS |

## 复算统计

| group | kind | count | median | min | max |
|---|---|---:|---:|---:|---:|
| type_query | recursive | 32 | 0.0940 | 0.0930 | 0.1100 |
| type_query | fold | 32 | 0.0940 | 0.0940 | 0.1250 |
| type_query | recursive | 128 | 0.1100 | 0.1090 | 0.1410 |
| type_query | fold | 128 | 0.1090 | 0.0940 | 0.1250 |
| type_query | recursive | 256 | 0.1570 | 0.1400 | 0.1720 |
| type_query | fold | 256 | 0.1090 | 0.0930 | 0.1250 |
| multi_tu_compile | implicit |  | 0.3750 | 0.3440 | 0.4220 |
| multi_tu_compile | explicit |  | 0.4380 | 0.4220 | 0.4530 |

## section headline

| kind | caller obj text raw | all obj text raw | exe text virtual |
|---|---:|---:|---:|
| implicit | 336 | 705 | 90861 |
| explicit | 28 | 474 | 90861 |

## 只读验证

- `python -B -m py_compile C04_Generic_CompileTime_Reflection/references/benchmarks/cost_driver.py`：PASS。
- pattern scan：无空 catch、硬编码 `apiKey`、`best effort`、`silent`、`fallback` 命中。
- 没有启动 compiler，没有重新运行 full benchmark。
