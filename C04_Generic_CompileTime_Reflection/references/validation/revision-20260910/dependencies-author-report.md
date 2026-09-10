# C04 依赖准备作者报告

结论：C04 Mp11/Hana 固定依赖准备已完成，接口可供后续 U01/U02 或样章接线使用。

已实现：

- `exercises/tools/prepare_meta_libraries.ps1`：下载并验证 `mp11-boost-1.91.0` 与 `hana-boost-1.91.0`，写入 `.learncpp-dependency.json` 和 `.learncpp-dependency.cmake`。
- `exercises/cmake/MetaLibraries.cmake`：提供 `c04_link_mp11(target)` 与 `c04_link_hana(target)`。

固定路径：

- `exercises/build/_deps/mp11-boost-1.91.0/source`
- `exercises/build/_deps/hana-boost-1.91.0/source`

验证证据：

- `dependencies-meta-20260910-123124/summary.json`：脚本准备 PASS；记录 remote、commit、license marker、SHA-256、header marker。
- `dependencies-cmake-20260910-123814/summary.json`：CMake 正负例 PASS；覆盖 Mp11/Hana 正例 configure/build/run、开关关闭、缺依赖、stale marker。

边界：

- CMake 配置期只做本地校验，不联网。
- marker commit 之外还核对本地 `git rev-parse HEAD` 和 tracked dirty 状态。
- Boost.Mp11 本独立仓库没有单独 license 文件；记录含 license 声明的 `README.md` 作为 license marker。
