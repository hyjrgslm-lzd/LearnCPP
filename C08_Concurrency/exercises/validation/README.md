# C08 学生验证资产

`tools/verify_students.py` 会把 `C08_Concurrency/exercises` 复制到临时构建副本，只对目标题目的叶目录运行原有 CMake，并执行真实学生入口。它不修改其它题目的内容，不依赖 root exercises CMake 的题目扫描，也不修改共享 `StudySetup.cmake` 或 root/runtime CMake。

验证模式：

- `initial`：干净 starter 必须能构建，并且以 exit 1 命中固定 starter/TODO 诊断；非 0 但诊断不匹配、77/SKIP、崩溃、超时、sanitizer 报告都不算通过。
- `good`：注入公开 good overlay。实现必须走 Student 入口，不能 include 或实际依赖 `reference.hpp`、`solution.cpp`、`validation/`。
- `bad`：先生成完整 good，再替换一个真实行为点。期望结果固定为 exit 1，并且 stdout/stderr 中必须出现对应检查器诊断；超时、崩溃、ASan/TSan/UBSan/LSan 报告、直接 `return 1` 都不算通过。

脚本使用本目录已有 `tools/run_benchmarks.py::run_process` 执行外部超时和 Windows 进程树清理。每个 configure/build/run 的完整 stdout、stderr、JSON 原始输出都会保存在对应 `logs/` 目录。配置时同时传 `-DCMAKE_BUILD_TYPE=<config>` 和 `cmake --build --config <config>`：Ninja/Make 等单配置生成器获得 Release flags，多配置 Visual Studio/Xcode 仍按 `--config` 构建。good 模式还会强制编译器输出 include trace：Windows 默认 `/showIncludes`，其它平台默认 `-H`，可用 `C08_STUDENT_VALIDATION_INCLUDE_FLAG` 覆盖；脚本解析 `/showIncludes`、`-H`、depfile/tlog 的真实依赖，完整依赖写入 `dependencies.txt`，控制台只打印依赖数量、hash 和课程内依赖片段。

如果 `--build-root` 已存在且非空，脚本会自动在其下创建新的 `run-YYYYMMDD-HHMMSS-ffffff` 子目录，避免覆盖旧 `course/`、`build/`、`logs/`、`summary.json`。

L3/Capstone3 的公共 `checks.hpp` 只放输入生成、类型/shape helper 和独立 oracle；`reference.hpp` 保留答案和 benchmark 驱动。L3 good 只复用公共遍历接口 `cs::numeric::map_with`；Capstone3 good 独立实现 mandatory 的 naive/tiled/threaded/par GEMM、plain/par/threaded reduction 和 sort，不调用 Reference 的 `gemm_*`、`sum_*` 或 `sort_values` 答案核心。Capstone3 optional SIMD 不在 good overlay 中冒充完成；`--with-simd` 会普通失败并提示用 Reference 做 SIMD 检查。

示例：

```powershell
python tools/verify_students.py --build-root ..\build\student-validation --target B1_mutex_family
python tools/verify_students.py --build-root ..\build\student-validation
```
