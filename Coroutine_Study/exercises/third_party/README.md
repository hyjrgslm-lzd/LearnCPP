# third_party 离线依赖

stage 3 的题目（模块 H / I / J + 两个第三阶段结课）需要以下第三方依赖：

| 依赖           | 用途                       | 默认获取方式                       | 平台限制       |
| -------------- | -------------------------- | ---------------------------------- | -------------- |
| stdexec        | 模块 H：协程↔sender 桥接   | FetchContent（NVIDIA/stdexec）     | 全平台         |
| asio           | 模块 I-1：真实异步 IO      | FetchContent（chriskohlhoff/asio） | 全平台         |
| folly          | 模块 I-2：folly::coro      | find_package                       | Linux/macOS 优先 |
| liburing       | 模块 I-3：io_uring         | pkg-config                         | 仅 Linux       |
| Boost.Cobalt   | 模块 I-4：Boost 协程       | find_package(Boost cobalt)         | 仅非 Windows   |

## 离线放置

如果机器无法访问 GitHub，可以预先把仓库克隆到本目录下：

```bash
cd exercises/third_party
git clone https://github.com/NVIDIA/stdexec.git
git clone https://github.com/chriskohlhoff/asio.git
```

然后修改 `cmake/ThirdPartySetup.cmake`：把对应的 `FetchContent_Declare` 段改为
`SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/stdexec` 这样的本地路径。

## 锁定版本

把 `GIT_TAG main` 改为具体的 commit hash 即可。建议在团队/课程环境中固定版本，避免上游变动导致用例失败。

## 跳过 stage 3

如果你暂时只想做 stage 1/2，配置时加：

```bash
cmake --preset default -DCOROUTINE_STUDY_ENABLE_STAGE3=OFF
```

stage 3 子目录将不被纳入构建，本目录里也不需要任何依赖。
