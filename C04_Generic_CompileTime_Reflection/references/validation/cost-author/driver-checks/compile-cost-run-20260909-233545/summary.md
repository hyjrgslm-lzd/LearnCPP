# C04 B01 编译成本实验结果

记录时间：`2026-09-09T15:35:45.845316+00:00`。随机种子：`40412025`。

## 环境

- 编译器：`D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin\clang-cl.exe`，sha256 `ec094d5f8f37c106eefcfc47c9997c261a06e3bd8eaaa7255835983a58524156`
- 标准与选项：`/std:c++latest /O2 /EHsc /GR-`；trace 额外使用 `-ftime-trace` 和 `-ftime-trace-granularity=0`。
- 进程空闲检测：按进程名/PID 观察 `MSBuild.exe, cl.exe, clang-cl.exe, cmake.exe, link.exe, lld-link.exe, ninja.exe`；不读取完整 command line。

## 类型查询正式计时

| 项数 | 版本 | 有效样本秒 | 中位数 | 范围 |
|---:|---|---|---:|---|
| 32 | recursive | INVALID | INVALID | INVALID-INVALID |
| 32 | fold | INVALID | INVALID | INVALID-INVALID |
| 128 | recursive | INVALID | INVALID | INVALID-INVALID |
| 128 | fold | INVALID | INVALID | INVALID-INVALID |
| 256 | recursive | INVALID | INVALID | INVALID-INVALID |
| 256 | fold | INVALID | INVALID | INVALID-INVALID |

## 类型查询 trace 定位

| 项数 | 版本 | Total Frontend us | Total InstantiateClass count | Total InstantiateFunction count |
|---:|---|---:|---:|---:|
| 32 | recursive | 21429 | 6 | 4 |
| 32 | fold | 20484 | 3 | 4 |
| 128 | recursive | 38701 | 6 | 4 |
| 128 | fold | 25356 | 3 | 4 |
| 256 | recursive | 109875 | 6 | 4 |
| 256 | fold | 27542 | 3 | 4 |

## 多 TU 正式计时

| 版本 | 有效样本秒 | 中位数 | 范围 |
|---|---|---:|---|
| implicit | INVALID | INVALID | INVALID-INVALID |
| explicit | INVALID | INVALID | INVALID-INVALID |

## 多 TU section 证据

| 版本 | 调用方 .obj .text 合计 | 全部 .obj .text 合计 | exe .text |
|---|---:|---:|---:|
| implicit | 336 | 705 | 90861 |
| explicit | 28 | 474 | 90861 |

## 边界

- trace 是定位证据，不与正式时间混计。
- trace 事件有嵌套关系，表格只列 `Total ...` 计数和持续时间，不累计成 CPU 占比。
- 空闲检测只按构建进程名/PID，不能排除普通系统负载。
- 无效样本保留在 `raw.json`，不进入中位数。
