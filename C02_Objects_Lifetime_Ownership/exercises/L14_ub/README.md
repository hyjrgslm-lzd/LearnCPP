# 练习 L14：行为分类与诊断边界

先阅读 [14 未定义行为、诊断与优化](../../chapters/14-undefined-behavior-and-optimization.md)。

本题默认只运行安全分类和不触发 UB 的检查。真实悬空、越界、未初始化读取不在默认 target 中执行。

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L14_ub -B build/c02-l14 -G "Visual Studio 18 2026" -A x64
cmake --build build/c02-l14 --config Debug
ctest --test-dir build/c02-l14 -C Debug --output-on-failure
```

解析：UB、IFNDR、implementation-defined、unspecified 和 C++26 erroneous behavior 的证据口径不同。一次运行通过只证明当前安全路径；不证明真实 UB 不存在。

前沿能力入口默认关闭。显式启用 `CORE_STUDY_ENABLE_FRONTIER=ON` 后，CTest 会通过当前 CMake generator/compiler 构建固定 compile-only probe，并把命令、stdout、stderr、状态写入 build 目录：

- frontier probe target 使用 `EXCLUDE_FROM_ALL`，普通 `cmake --build --preset frontier` 不会因为某个未支持能力而失败；能力探测由 CTest 单独构建对应 target。
- 每个 probe 输出唯一 `frontier/<case>-<timestamp>.txt`，同时 append 到 `frontier/index.txt`；旧输出不会被覆盖。
- 每个 CTest probe 带 `RESOURCE_LOCK`，避免并发测试同时构建同一 build tree。
- baseline aggregate / `bit_cast` 必须编译；失败是工具链或配置失败。
- P2287R6 base/indirect member designated initializer：支持则 PASS；不支持时必须匹配本 case 源码和 designator 相关诊断才记 capability SKIP。probe 使用 `Derived{.base = 1, .member = 2}`，不使用基类名作 designator。
- P2748 return temporary reference：若编译器用本 case 源码和 return-temporary 诊断拒绝则 PASS；若只诊断后接受，记录为 SKIP，不能据此证明 C++26 语义已实现。
- P2953 defaulted assignment restriction：若用本 case 源码和 defaulted-assignment/ref-qualified 相关诊断拒绝则 PASS；若接受则 SKIP，表示当前工具链未执行该限制。
- provenance / invalid pointer / lifetime-end 相关 DR 只提供 compile-only review model，不声称 runtime 支持。

UNSAFE 入口也默认关闭。只有同时启用 `CORE_STUDY_ENABLE_ASAN=ON` 和 `CORE_STUDY_ENABLE_UNSAFE_DEMOS=ON` 时，才构建并注册 `L14_ub_asan_uaf_diagnostic`。它是独立诊断进程，期望非零退出并匹配 `heap-use-after-free` 与 `asan_uaf_probe.cpp`；正常课程 preset 不运行真实 UB。
