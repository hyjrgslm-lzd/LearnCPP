# C05 修订依赖契约

`DATA_STUDY_ENABLE_FORMAT_LIBS` 默认 `OFF`。C05 核心配置必须保持离线，不依赖 fmt 或 spdlog。需要这些库的单元包含 `exercises/cmake/FormatLibraries.cmake` 后调用：

- `c05_link_fmt(target)`：接入 fmt。
- `c05_link_spdlog(target)`：接入 spdlog。

启用这些单元前，先运行：

```powershell
./tools/prepare_format_libraries.ps1
```

脚本只下载到 `exercises/build/_deps`。已有 checkout 必须 remote、commit、工作区状态匹配；脏输入、错误 remote、错误 commit 都会失败，不覆盖本地内容。脚本记录 remote URL、固定 commit、license path、license SHA-256、源码路径和头文件 marker；运行记录留在本地未跟踪目录。

CMake 接线会在配置期重新核对 marker、源码路径边界、本地 `git rev-parse HEAD` 和 tracked dirty 状态。若 `fmt::fmt` 或 `spdlog::spdlog` 已存在，也必须是本课固定源码生成的静态库；外部预定义 target 会失败。

固定输入：

| Library | Remote | Commit | Local prefix |
| --- | --- | --- | --- |
| fmt | `https://github.com/fmtlib/fmt.git` | `407c905e45ad75fc29bf0f9bb7c5c2fd3475976f` | `exercises/build/_deps/fmt-12.1.0` |
| spdlog | `https://github.com/gabime/spdlog.git` | `79524ddd08a4ec981b7fea76afd08ee05f83755d` | `exercises/build/_deps/spdlog-1.17.0` |

`DATA_STUDY_SPDLOG_BACKEND=fmt` 构建静态 `fmt::fmt` 和静态 `spdlog::spdlog`，并固定：

- `SPDLOG_FMT_EXTERNAL=ON`
- `SPDLOG_FMT_EXTERNAL_HO=OFF`
- `SPDLOG_USE_STD_FORMAT=OFF`

`DATA_STUDY_SPDLOG_BACKEND=std` 构建静态 `spdlog::spdlog`，并固定：

- `SPDLOG_USE_STD_FORMAT=ON`
- `SPDLOG_FMT_EXTERNAL=OFF`
- `SPDLOG_FMT_EXTERNAL_HO=OFF`
