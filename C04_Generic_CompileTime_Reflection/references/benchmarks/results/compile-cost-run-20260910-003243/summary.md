# C04 B01 编译成本实验结果

模式：`full`。记录时间：`2026-09-09T16:32:43.031411+00:00`。随机种子：`40412025`。

## 环境

- 编译器：`D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin\clang-cl.exe`，sha256 `ec094d5f8f37c106eefcfc47c9997c261a06e3bd8eaaa7255835983a58524156`
- clang 版本：`clang version 22.1.3 (https://github.com/llvm/llvm-project e9846648fd6183ee6d8cbdb4502213fcf902a211)`
- CPU：`{"Name":"AMD Ryzen 9 9900X 12-Core Processor            ","NumberOfCores":12,"NumberOfLogicalProcessors":24}`
- OS：`Windows 10 10.0.26100`，machine `AMD64`。
- 标准与选项：`/std:c++latest /O2 /EHsc /GR-`；trace 额外使用 `-ftime-trace` 和 `-ftime-trace-granularity=0`。
- 进程空闲检测：按进程名/PID/CPU 观察 `MSBuild.exe, cl.exe, clang-cl.exe, cmake.exe, link.exe, lld-link.exe, ninja.exe`；不读取完整 command line。
- 多 TU 计时责任：implicit 为 4 个调用方编译进程；explicit 为 4 个调用方加 1 个 provider/instantiation 编译进程。main 编译与 link 只进入 correctness/section 绑定，不进入正式编译时间样本。
- 产物 SHA-256 在编译/链接命令结束后读取，不计入 `process_seconds`。

## 样本门

| 组 | 版本 | 项数 | 成功预热 | 有效样本 | 状态 |
|---|---|---:|---:|---:|---|
| type_query | recursive | 32 | 1 | 5 | PASS |
| type_query | fold | 32 | 1 | 5 | PASS |
| type_query | recursive | 128 | 1 | 5 | PASS |
| type_query | fold | 128 | 1 | 5 | PASS |
| type_query | recursive | 256 | 1 | 5 | PASS |
| type_query | fold | 256 | 1 | 5 | PASS |
| multi_tu_compile | implicit |  | 1 | 5 | PASS |
| multi_tu_compile | explicit |  | 1 | 5 | PASS |

## 类型查询正式计时

| 项数 | 版本 | 有效样本秒 | 中位数 | 范围 |
|---:|---|---|---:|---|
| 32 | recursive | 0.0940, 0.0940, 0.1100, 0.0940, 0.1090 | 0.0940 | 0.0940-0.1100 |
| 32 | fold | 0.1090, 0.0940, 0.0940, 0.0780, 0.0930 | 0.0940 | 0.0780-0.1090 |
| 128 | recursive | 0.1090, 0.1250, 0.1090, 0.1100, 0.1090 | 0.1090 | 0.1090-0.1250 |
| 128 | fold | 0.0940, 0.1100, 0.1560, 0.0940, 0.1250 | 0.1100 | 0.0940-0.1560 |
| 256 | recursive | 0.1560, 0.1410, 0.1410, 0.1560, 0.1410 | 0.1410 | 0.1410-0.1560 |
| 256 | fold | 0.1090, 0.0940, 0.1090, 0.0930, 0.1090 | 0.1090 | 0.0930-0.1090 |

## 类型查询 trace 定位

| 项数 | 版本 | Total Frontend us | Total InstantiateClass count | Total InstantiateFunction count |
|---:|---|---:|---:|---:|
| 32 | recursive | 21228 | 6 | 4 |
| 32 | fold | 18940 | 3 | 4 |
| 128 | recursive | 33602 | 6 | 4 |
| 128 | fold | 22194 | 3 | 4 |
| 256 | recursive | 78501 | 6 | 4 |
| 256 | fold | 23314 | 3 | 4 |

## 多 TU 正式计时

| 版本 | 有效样本秒 | 中位数 | 范围 |
|---|---|---:|---|
| implicit | 0.4530, 0.3280, 0.3430, 0.4220, 0.3590 | 0.3590 | 0.3280-0.4530 |
| explicit | 0.3900, 0.4070, 0.4060, 0.4060, 0.4370 | 0.4060 | 0.3900-0.4370 |

## 多 TU section 证据

| 版本 | 调用方 .obj .text raw 合计 | 全部 .obj .text raw 合计 | exe .text virtual |
|---|---:|---:|---:|
| implicit | 336 | 705 | 90861 |
| explicit | 28 | 474 | 90861 |

## 边界

- trace 是定位证据，不与正式时间混计。
- trace 事件有嵌套关系，表格只列 `Total ...` 计数和持续时间，不累计成 CPU 占比。
- 空闲检测按构建进程名/PID/CPU 增量判断；完全发生在两次快照之间的短任务仍可能漏检，普通 OS 噪声不排除。
- 无效样本保留在 `raw.json`，不进入中位数。
