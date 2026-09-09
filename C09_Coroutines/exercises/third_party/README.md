# third_party 离线依赖

默认 `student` / `verify-core` 不需要本目录，也不会联网。

启用 light 依赖并允许 FetchContent：

```bash
cmake --preset full-linux-light
cmake --preset full-windows
```

离线时把依赖放在任意目录，再用 CMake 标准覆盖变量指向源码：

```bash
cmake --preset full-linux-light \
  -DFETCHCONTENT_SOURCE_DIR_STDEXEC=/deps/stdexec \
  -DFETCHCONTENT_SOURCE_DIR_ASIO=/deps/asio \
  -DFETCHCONTENT_SOURCE_DIR_CPPCORO=/deps/cppcoro
```

固定版本：

| 依赖 | 获取方式 | 固定版本 |
| --- | --- | --- |
| stdexec | FetchContent / `FETCHCONTENT_SOURCE_DIR_STDEXEC` | `nvhpc-26.05` |
| Asio | FetchContent / `FETCHCONTENT_SOURCE_DIR_ASIO` | `asio-1-38-2` |
| cppcoro | FetchContent / `FETCHCONTENT_SOURCE_DIR_CPPCORO` | `8642e98596a92be30a2b061d3ed306d959d3214e` |
| liburing | pkg-config | 2.15+ |
| Folly | 外部安装 + `find_package(Folly CONFIG)` | `v2026.08.31.00` |
| Boost.Cobalt | 外部 Boost + `find_package(Boost 1.92 COMPONENTS cobalt)` | 1.92 |

显式打开某依赖后，如果 CMake 找不到它，会 `FATAL_ERROR`。这是故意的：避免某题被静默跳过。

cppcoro 不是纯 header-only 依赖；CMake 会接入它的真实 `cppcoro` target，再桥接为 `cppcoro::cppcoro`。
