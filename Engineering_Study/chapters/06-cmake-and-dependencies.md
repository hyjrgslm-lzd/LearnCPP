# 06：CMake target、配置与依赖

C++ 工程最终交给编译器的是一组编译命令和链接命令。CMake 的核心价值不是把命令藏起来，而是把“谁需要什么”记录在 target 图里，然后为当前生成器写出可执行的构建图。读 CMake 时先找 target，再看 target 的源码、include、宏、编译选项和链接依赖怎样传播。

## Target 是构建图节点

`add_library(core ...)`、`add_executable(app ...)` 和 `add_library(policy INTERFACE)` 都创建 target。区别在于 target 是否产生二进制产物，以及它的使用要求给谁用。

三个作用域关键字描述“这条要求给谁用”：

- `PRIVATE`：只给当前 target 编译或链接自己时用。实现 `.cpp` 需要的宏、内部 include 目录、实现细节库通常是 `PRIVATE`。
- `PUBLIC`：当前 target 自己用，也传给消费者。公开头文件中出现的 include 目录、公开 API 所需的 feature、影响 ABI 布局的宏，必须是 `PUBLIC`。
- `INTERFACE`：当前 target 自己不用，只作为使用要求传给消费者。注意它是作用域，不等同于 `INTERFACE library` 这种 target 类型。普通 static/shared library 也可以有 `INTERFACE` 要求；`add_library(x INTERFACE)` 创建的是没有源码、不编译自己的纯传播 target。

看一个正反对照：

```cmake
add_library(f1_math src/math.cpp)
target_include_directories(f1_math PUBLIC include)
target_compile_definitions(f1_math PRIVATE F1_BUILDING_LIBRARY=1)
target_compile_definitions(f1_math INTERFACE F1_CONSUMER_SEES_API=1)

target_link_libraries(app PRIVATE f1_math)
```

`f1_math` 编译自己的 `.cpp` 时会看到 `include` 和 `F1_BUILDING_LIBRARY`，不会因为 `INTERFACE F1_CONSUMER_SEES_API` 而给自己额外定义这个宏。`app` 链接 `f1_math` 后会继承 `include` 和 `F1_CONSUMER_SEES_API`，但不会继承 `F1_BUILDING_LIBRARY`。这就是 `INTERFACE` 作用域的真实含义：使用要求只供消费者，不给当前 target 自己；它不是“这个 target 当前不编译”的同义词。

再看常见错误：公开头 `math.hpp` 里包含 `provider/provider.hpp`，但 CMake 写成：

```cmake
target_link_libraries(f1_math PRIVATE provider)
```

`f1_math.cpp` 能编译，因为它自己能看到 provider。消费者包含 `math.hpp` 时却找不到 provider 头。修复不是在 `app` 手写 include，而是把依赖语义放回提供 API 的 target：

```cmake
target_link_libraries(f1_math PUBLIC provider)
```

这样依赖图表达了真实接口：使用 `f1_math` 的人也必须能使用 provider 的公开头和公开编译要求。

## 生成器表达式和配置

生成器表达式在生成构建系统时求值，用 `$<...>` 表示。它用于表达同一个 target 在不同配置、编译器、平台下采用不同要求：

```cmake
target_compile_definitions(core PUBLIC
    $<$<CONFIG:Debug>:CORE_DEBUG_BUILD>
    $<$<CXX_COMPILER_ID:MSVC>:CORE_ON_MSVC>
)
```

这不是 C++ 运行期 `if`，也不是 configure 阶段普通变量替换。它写进构建图，直到具体配置和生成器确定后才展开。

单配置生成器如 Ninja 通常由 `CMAKE_BUILD_TYPE=Release` 在 configure cache 中决定配置。多配置生成器如 Visual Studio 在 build 阶段用 `--config Release` 选择配置。把 `CMAKE_BUILD_TYPE=Debug` 写进 Visual Studio 多配置构建，通常不能控制实际编译配置。反过来，在单配置 Ninja 下只写 `cmake --build build --config Debug` 也不能替代 configure 时的 `CMAKE_BUILD_TYPE=Debug`。

## Cache、preset 和 toolchain file

CMake configure 阶段生成 `CMakeCache.txt`。cache 记录用户选择、探测结果和工具路径。改 cache 变量后需要重新 configure；只改 `.cpp` 通常只需要 build。不要把旧 cache 的成功当作新工具链能力证明，因为 `CMAKE_CXX_COMPILER`、标准库路径、feature probe 都可能来自旧 configure。

Preset 是可复现入口：把 generator、build dir、cache 变量和环境约定写成名字。课程 preset 应表达“要验证什么”，不应硬塞某个作者机器的临时路径。机器相关路径应放在本机命令记录或 validation 说明中。

Toolchain file 在 `project()` 启用语言前读取，用来指定编译器、sysroot、目标平台和查找策略。交叉编译时，构建机和目标机分离：编译器可能在 Windows 上运行，产物给嵌入式 Linux 用。此时 `try_run` 不能默认执行目标程序；`find_package` 也必须区分 host 工具和 target 库。

`sysroot` 是目标系统头文件和库的根。它不是 include 目录列表的别名，而是告诉编译器和查找逻辑目标系统长什么样。错误的 sysroot 会让代码在构建机上编译通过，却链接到目标机没有的库，或反过来误用构建机库。

## 固定依赖：来源、版本、许可证、消费方式

依赖进入课程前要回答四件事：

1. 来源：仓库内 fixture、本地安装、系统 SDK、固定 release 包或固定 commit。
2. 版本：能被重新找到的 tag/hash/包版本。
3. 许可证：是否允许学习分发、修改和示例引用。
4. 消费方式：`find_package`、subdirectory、`FetchContent`、包管理器或手写路径。

`find_package` 适合已经安装或已经导出的包。现代 CMake 包应导入 target，例如 `Provider::provider`，消费者链接 target，而不是手写 include/lib 路径。版本不匹配应在 configure 阶段失败，这比链接时找不到符号更早、更清楚。

`FetchContent` 在 configure 阶段把源码带入当前构建。它适合固定 commit 的源码依赖，但引入网络、缓存和供应链边界。课程默认不下载依赖；`F2_dependencies` 使用仓库内 offline provider，同时给出两条可验证路径：

- `find_package`：先把 fixture 安装到本地 build prefix，再由 consumer 按 `Provider_DIR` 或 `CMAKE_PREFIX_PATH` 查找。
- `FetchContent` offline：`F2_PROVIDER_FIXTURE_SOURCE_DIR` 默认指向仓库内固定源码目录，不访问网络；显式覆盖时也必须指向本地源码树。

这不是把 J1 后续课程的小库伪装成第三方生态。F2 的 provider 是离线教学 fixture，许可证、版本和源码都在本练习内固定；真实第三方依赖需要额外审许可证、上游 tag、镜像策略和安全更新边界。

## 练习连接

`F1_cmake_targets` 验证 target 使用要求：Reference target 正确传播 include 和 consumer 宏；Student target 故意缺少接口语义，启用 student 测试后会暴露。

`F2_dependencies` 验证固定 offline provider：Reference 走导入 target 或 offline source，缺包和版本不匹配是明确 configure 失败，不用任意编译错误冒充依赖管理测试。

## 自测

- 公开头文件包含另一个库的头，当前 target 应该把那个库放在 `PRIVATE` 还是 `PUBLIC`？
- `target_compile_definitions(lib INTERFACE X=1)` 对 `lib` 自己和消费者分别生效吗？
- 一个 warning policy target 没有源码，只传递 `/W4` 或 `-Wall`，它应是什么 target 类型？
- Ninja 下 `CMAKE_BUILD_TYPE=Debug` 和 Visual Studio 下 `--config Debug` 的区别是什么？
- 为什么固定依赖时只写“用最新版本”不够？

答案：公开头需要的库是 `PUBLIC`；`INTERFACE` 要求只给消费者，不给当前 target 自己；无源码只传播要求的是 `INTERFACE library`；Ninja 是 configure cache 选择单配置，Visual Studio 是 build 阶段选择多配置；“最新版本”不可复现，不能绑定接口、许可证、bug 边界或安全修复状态。
