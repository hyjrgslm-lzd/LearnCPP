# C05 依赖准备作者报告

结论：C05 fmt/spdlog 固定依赖准备已完成，接口可供 `U02_fmt`、`U03_spdlog` 或样章接线使用。

已实现：

- `exercises/tools/prepare_format_libraries.ps1`：下载并验证 `fmt-12.1.0` 与 `spdlog-1.17.0`，写入 `.learncpp-dependency.json` 和 `.learncpp-dependency.cmake`。
- `exercises/cmake/FormatLibraries.cmake`：提供 `c05_link_fmt(target)` 与 `c05_link_spdlog(target)`。

固定路径：

- `exercises/build/_deps/fmt-12.1.0/source`
- `exercises/build/_deps/spdlog-1.17.0/source`

验证证据：

- `dependencies-format-20260910-123030/summary.json`：脚本准备 PASS；记录 remote、commit、license path、SHA-256、header marker。
- `../../../../C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/dependencies-cmake-20260910-123814/summary.json`：CMake 正负例 PASS；覆盖 fmt backend、std backend、非法 backend、错误 commit marker、外部预定义 `fmt::fmt` target 污染。

边界：

- CMake 配置期只做本地校验，不联网。
- marker commit 之外还核对本地 `git rev-parse HEAD` 和 tracked dirty 状态。
- `fmt::fmt` 与 `spdlog::spdlog` 必须是本课固定源码生成的静态库；已有同名外部 target 会失败。
