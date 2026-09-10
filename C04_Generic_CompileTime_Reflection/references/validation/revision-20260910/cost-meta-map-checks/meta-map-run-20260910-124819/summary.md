# C04 B01 元 map 查找成本实验结果

模式：`meta-map-check-only`。记录时间：`2026-09-10T04:48:19.698149+00:00`。随机种子：`40412027`。

## 环境

- 编译器：`D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin\clang-cl.exe`，sha256 `ec094d5f8f37c106eefcfc47c9997c261a06e3bd8eaaa7255835983a58524156`
- clang 版本：`clang version 22.1.3 (https://github.com/llvm/llvm-project e9846648fd6183ee6d8cbdb4502213fcf902a211)`
- Mp11：`F:\CPPTrain\LearnCPP\C04_Generic_CompileTime_Reflection\exercises\build\_deps\mp11-boost-1.91.0\source`，commit `b94b089d4ec83cd397f20958f34edf25bc3e06f4`，期望 `b94b089d4ec83cd397f20958f34edf25bc3e06f4`。
- 标准与选项：`/std:c++latest /O2 /EHsc /GR-`；manual 与 Mp11 使用同一 TU 契约、同一 `source_map`、同一 `entry_value` 适配、同一 Boost.Mp11 头依赖、同一 include path、同一编译选项。
- 计时 clock：`time.perf_counter()` implementation `QueryPerformanceCounter()`，resolution `1e-07`。
- `perf_counter_seconds` 包含 Python runner、进程创建、clang-cl 编译和等待返回；源码/产物哈希在命令结束后记录，不计入该值。
- trace 插桩只用于定位，正式计时不用 trace。

## Check-only 结果

- detector 控制：`PASS`。
- 12 个正例先编译、链接、运行：manual 递归 map_find 与 Mp11 `mp_map_find` 输出相等；缺键为 `void`。

## Trace 定位

| 项数 | 查询 | 版本 | Total Frontend us | Total InstantiateClass count | Total EvaluateAsConstantExpr count |
|---:|---|---|---:|---:|---:|
| 32 | last | manual | 62479 | 30 | 528 |
| 32 | last | mp11 | 58998 | 30 | 464 |
| 32 | missing | manual | 59884 | 29 | 528 |
| 32 | missing | mp11 | 59373 | 29 | 464 |
| 128 | last | manual | 74137 | 30 | 720 |
| 128 | last | mp11 | 71137 | 30 | 464 |
| 128 | missing | manual | 70135 | 29 | 720 |
| 128 | missing | mp11 | 64009 | 29 | 464 |
| 256 | last | manual | 107133 | 30 | 976 |
| 256 | last | mp11 | 63229 | 30 | 464 |
| 256 | missing | manual | 109895 | 29 | 976 |
| 256 | missing | mp11 | 70861 | 29 | 464 |

## 边界

- 本实验测的是“手写递归 type map 查找”与 Boost.Mp11 `mp_map_find` 的编译成本，不测运行时吞吐。
- manual/Mp11 的输入类型、输出适配和 Boost.Mp11 头解析成本相同；本实验隔离 `manual_find` 与 `mp_map_find` 的查找机制形状。
- 0 有效样本会失败；所有预热、无效样本、命令、哈希和 probe 记录保存在 `raw.json`/`events.jsonl`。
