# 10：安装、导出与兼容消费

能在当前 build 目录运行，不等于能交付给另一个项目。真正的包消费至少要回答四个问题：头文件从哪里来，库文件从哪里来，使用要求怎样传给 consumer，consumer 离开原源码和原 build 后是否还能找到正确产物。

本章只做 header 包和普通静态/动态库包。named module 的 BMI、扫描和安装消费由 H1 负责；这里不把模块包问题混进来。

## build tree 不是安装包

CMake build 目录里有中间对象、生成文件、绝对路径和当前机器状态。consumer 如果直接包含 `../../E1_abi/include` 或链接某个 build 目录下的 `.lib`，它通过只能证明本机目录还在，不能证明包可交付。

J1 的检查先把库安装到专用 `prefix_A`，再复制成 `prefix_B`。独立 consumer 只设置 `CMAKE_PREFIX_PATH=prefix_B`，然后调用：

```cmake
find_package(LessonPackage 1.0.0 CONFIG REQUIRED)
target_link_libraries(app PRIVATE LessonPackage::lesson_static)
```

动态版本链接 `LessonPackage::lesson_shared`。导出目标只有这两个；没有额外 alias，避免 consumer 依赖一个没教学意义的名字。

## 使用要求必须跟着 target

静态库和动态库使用同一个 `lesson_api.h`，但宏不同。静态消费必须定义 `LESSON_STATIC`，让 `LESSON_API` 为空。动态库生产者编译 DLL 时定义 `LESSON_BUILD_SHARED`，头文件给导出函数加 `dllexport`；动态 consumer 不定义 `LESSON_STATIC`，在 Windows 上看到的是 `dllimport`。

这类信息不能只写在 README 里。CMake exported target 应该把 include 目录和必要 compile definitions 带给 consumer。J1 会分别构建 static 和 shared consumer，验证头定义传播有效。

## 版本文件拒绝不兼容请求

包版本固定为 `1.0.0`。`find_package(LessonPackage 1.0.0 CONFIG REQUIRED)` 应该通过；请求 `2.0.0` 应该在 configure 阶段失败。这个失败说明 CMake 包版本文件在工作，不说明 ABI 本身永远兼容。ABI 版本仍由 `lesson_abi_version()` 和 `lesson_create(requested_abi, ...)` 在运行时拒绝。

构建期包版本和运行期 ABI 版本是两层门。前者阻止 consumer 用明显不满足要求的包配置；后者让程序面对实际 DLL 时还能自检。

## 重定位检查

安装包常见错误是导出的 include 目录仍指向源码目录或 build 目录。这样的包在作者机器上能用，复制到另一处就坏。J1 在复制 `prefix_A` 到 `prefix_B` 后检查导出文件内容，拒绝出现当前源码目录或 build 目录。

这个检查不是靠删除原源码来制造失败；它只读导出文件和从新 prefix 配置 consumer。复制和删除都限制在 J1 自己创建的 build stage 下，不碰用户文件和原源码。

## 动态库运行检查

Windows 上 consumer 链接 shared target 时，链接阶段使用导入库；运行阶段还要找到 DLL。J1 的 shared consumer 用子进程 `PATH` 临时加入 `prefix_B/bin`，不修改系统 PATH。缺 DLL 负例只删除 stage 副本里的 DLL，再运行已构建的 consumer，要求失败来自运行时装载，而不是 configure 或 build。

ELF 平台通常涉及 rpath、`LD_LIBRARY_PATH` 和 loader cache。本课保留条件路径，但本轮不把未运行的 ELF 路径写成通过。

## 失败对照

包检查至少要覆盖三类失败：

- 请求 `2.0.0` 被版本文件拒绝。
- consumer 链接不存在的 `LessonPackage::lesson` 被 configure/generate 阶段拒绝。
- shared consumer 缺 DLL 时运行失败。

这些失败不能混在一起。版本不匹配是 configure 失败；缺导出 target 是 CMake 目标解析失败；缺 DLL 是运行失败。检查器分别匹配阶段和关键文字，timeout 或无关错误不算通过。

## 自测

1. 为什么要复制到 `prefix_B` 后再消费？

   **解析：** 复制后 consumer 不能依赖原安装动作留下的偶然路径。只有从 `prefix_B` 的 package config、targets、include 和 lib/bin 成功配置、构建、运行，才能说明包具备基本重定位能力。

2. 为什么 static target 要传播 `LESSON_STATIC`？

   **解析：** Windows 静态消费不能把 API 声明成 `dllimport`。`LESSON_STATIC` 让 `LESSON_API` 为空，函数按普通静态链接符号处理。

3. 为什么缺 DLL 不是链接失败？

   **解析：** shared consumer 链接时使用导入库，链接器已经满足符号。DLL 是运行时装载器需要的文件；缺失时失败发生在进程启动或显式加载阶段。
