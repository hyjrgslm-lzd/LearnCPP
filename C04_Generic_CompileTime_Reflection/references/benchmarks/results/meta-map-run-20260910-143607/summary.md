# C04 B01 元 map 查找成本实验结果

模式：`meta-map`。记录时间：`2026-09-10T06:36:07.422222+00:00`。随机种子：`40412027`。

## 环境

- 编译器：`D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin\clang-cl.exe`，sha256 `ec094d5f8f37c106eefcfc47c9997c261a06e3bd8eaaa7255835983a58524156`
- clang 版本：`clang version 22.1.3 (https://github.com/llvm/llvm-project e9846648fd6183ee6d8cbdb4502213fcf902a211)`
- Mp11：`F:\CPPTrain\LearnCPP\C04_Generic_CompileTime_Reflection\exercises\build\_deps\mp11-boost-1.91.0\source`，commit `b94b089d4ec83cd397f20958f34edf25bc3e06f4`，期望 `b94b089d4ec83cd397f20958f34edf25bc3e06f4`。
- 标准与选项：`/std:c++latest /O2 /EHsc /GR-`；manual 与 Mp11 使用同一 TU 契约、同一 `source_map`、同一 `entry_value` 适配、同一 Boost.Mp11 头依赖、同一 include path、同一编译选项。
- 计时 clock：`time.perf_counter()` implementation `QueryPerformanceCounter()`，resolution `1e-07`。
- `perf_counter_seconds` 包含 Python runner、进程创建、clang-cl 编译和等待返回；源码/产物哈希在命令结束后记录，不计入该值。
- trace 插桩只用于定位，正式计时不用 trace。

## 样本门

| 组 | 版本 | 项数 | 查询 | 成功预热 | 有效样本 | 状态 |
|---|---|---:|---|---:|---:|---|
| meta_map | manual | 32 | last | 1 | 5 | PASS |
| meta_map | mp11 | 32 | last | 1 | 5 | PASS |
| meta_map | manual | 32 | missing | 1 | 5 | PASS |
| meta_map | mp11 | 32 | missing | 1 | 5 | PASS |
| meta_map | manual | 128 | last | 1 | 5 | PASS |
| meta_map | mp11 | 128 | last | 1 | 5 | PASS |
| meta_map | manual | 128 | missing | 1 | 5 | PASS |
| meta_map | mp11 | 128 | missing | 1 | 5 | PASS |
| meta_map | manual | 256 | last | 1 | 5 | PASS |
| meta_map | mp11 | 256 | last | 1 | 5 | PASS |
| meta_map | manual | 256 | missing | 1 | 5 | PASS |
| meta_map | mp11 | 256 | missing | 1 | 5 | PASS |

## 正式计时

| 项数 | 查询 | 版本 | 有效样本 perf_counter 秒 | 中位数 | 范围 |
|---:|---|---|---|---:|---|
| 32 | last | manual | 0.1461, 0.1385, 0.1335, 0.1303, 0.1452 | 0.1385 | 0.1303-0.1461 |
| 32 | last | mp11 | 0.1420, 0.1271, 0.1441, 0.1462, 0.1685 | 0.1441 | 0.1271-0.1685 |
| 32 | missing | manual | 0.1561, 0.1467, 0.1356, 0.1556, 0.1424 | 0.1467 | 0.1356-0.1561 |
| 32 | missing | mp11 | 0.1398, 0.1368, 0.1417, 0.1422, 0.1348 | 0.1398 | 0.1348-0.1422 |
| 128 | last | manual | 0.1716, 0.1568, 0.1440, 0.1664, 0.1438 | 0.1568 | 0.1438-0.1716 |
| 128 | last | mp11 | 0.2151, 0.2246, 0.1329, 0.1382, 0.1365 | 0.1382 | 0.1329-0.2246 |
| 128 | missing | manual | 0.1702, 0.1425, 0.1476, 0.1431, 0.1512 | 0.1476 | 0.1425-0.1702 |
| 128 | missing | mp11 | 0.1447, 0.1434, 0.1357, 0.2128, 0.1527 | 0.1447 | 0.1357-0.2128 |
| 256 | last | manual | 0.1532, 0.1901, 0.1743, 0.1606, 0.1514 | 0.1606 | 0.1514-0.1901 |
| 256 | last | mp11 | 0.1722, 0.1448, 0.1240, 0.1286, 0.1550 | 0.1448 | 0.1240-0.1722 |
| 256 | missing | manual | 0.1598, 0.1553, 0.1352, 0.1596, 0.1777 | 0.1596 | 0.1352-0.1777 |
| 256 | missing | mp11 | 0.1266, 0.1375, 0.1286, 0.1498, 0.1264 | 0.1286 | 0.1264-0.1498 |

## Trace 定位

| 项数 | 查询 | 版本 | Total Frontend us | Total InstantiateClass count | Total EvaluateAsConstantExpr count |
|---:|---|---|---:|---:|---:|
| 32 | last | manual | 59456 | 30 | 528 |
| 32 | last | mp11 | 61189 | 30 | 464 |
| 32 | missing | manual | 62164 | 29 | 528 |
| 32 | missing | mp11 | 58693 | 29 | 464 |
| 128 | last | manual | 77627 | 30 | 720 |
| 128 | last | mp11 | 69531 | 30 | 464 |
| 128 | missing | manual | 76622 | 29 | 720 |
| 128 | missing | mp11 | 63482 | 29 | 464 |
| 256 | last | manual | 102478 | 30 | 976 |
| 256 | last | mp11 | 69859 | 30 | 464 |
| 256 | missing | manual | 109880 | 29 | 976 |
| 256 | missing | mp11 | 68810 | 29 | 464 |

## 边界

- 本实验测的是“手写递归 type map 查找”与 Boost.Mp11 `mp_map_find` 的编译成本，不测运行时吞吐。
- manual/Mp11 的输入类型、输出适配和 Boost.Mp11 头解析成本相同；本实验隔离 `manual_find` 与 `mp_map_find` 的查找机制形状。
- 0 有效样本会失败；所有预热、无效样本、命令、哈希和 probe 记录保存在 `raw.json`/`events.jsonl`。
