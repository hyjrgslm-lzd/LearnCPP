# C04 修订依赖契约

`GENERIC_STUDY_ENABLE_META_LIBS` 默认 `OFF`。C04 核心配置必须保持离线，不依赖 Boost.Mp11 或 Boost.Hana。需要这些库的单元包含 `exercises/cmake/MetaLibraries.cmake` 后调用：

- `c04_link_mp11(target)`：接入 Boost.Mp11。
- `c04_link_hana(target)`：接入 Boost.Hana。

启用这些单元前，先运行：

```powershell
./tools/prepare_meta_libraries.ps1
```

脚本只下载到 `exercises/build/_deps`。已有 checkout 必须 remote、commit、工作区状态匹配；脏输入、错误 remote、错误 commit 都会失败，不覆盖本地内容。脚本记录 remote URL、固定 commit、license marker、license SHA-256、源码路径和头文件 marker；运行记录留在本地未跟踪目录。

CMake 接线会在配置期重新核对 marker、源码路径边界、本地 `git rev-parse HEAD` 和 tracked dirty 状态；不联网，也不 fallback 到系统 Boost。

固定输入：

| Library | Remote | Commit | Local prefix |
| --- | --- | --- | --- |
| Boost.Mp11 | `https://github.com/boostorg/mp11.git` | `b94b089d4ec83cd397f20958f34edf25bc3e06f4` | `exercises/build/_deps/mp11-boost-1.91.0` |
| Boost.Hana | `https://github.com/boostorg/hana.git` | `bc49ee25638e59d977edff5737b4e6bf12c1e5ea` | `exercises/build/_deps/hana-boost-1.91.0` |

说明：Boost.Mp11 独立仓库本 commit 没有单独 `LICENSE_1_0.txt`，其 `README.md` 明确声明 Boost Software License；脚本把该文件作为 license marker 记录 SHA。
