# 09：`import std` 的门控与实测边界

`import std;` 不是“把 `<vector>` 换个写法”。它需要语言前端、标准库模块接口、构建系统扫描、生成器 dyndep 和 target metadata 一起工作。任一层缺失，代码都不能稳定构建。

## 标准与实现层次

C++20 引入 named modules。C++23 标准库提供标准库命名模块的接口形式：`std` 暴露 C++ 标准库在 namespace `std` 中的声明，`std.compat` 还暴露部分 C 兼容全局名字。编译器支持 modules，不代表该标准库提供 `std` module；标准库提供 `std.ixx`，也不代表 CMake 会自动知道如何构建它。

因此课程分开验证两条路径：

- direct `cl`：手动编译 MSVC 自带 `std.ixx`，再用 `/reference std=std.ifc` 编译 consumer。
- CMake：设置 CMake 4.2.3 实验 gate，确认 `CMAKE_CXX_COMPILER_IMPORT_STD` 包含 `23`，给 target 开 `CXX_MODULE_STD ON`，让 CMake/Ninja 扫描、编译、映射并链接。

Direct 路径证明 MSVC 前端和 MSVC 标准库模块接口能工作。CMake 路径证明生成器集成也能工作。两者不能互相替代。

## CMake 4.2.3 gate

本课程固定 CMake 4.2.3。`import std` 实验 gate 必须在 `project()` 前设置：

```cmake
cmake_minimum_required(VERSION 4.2)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
project(i1_import_std LANGUAGES CXX)
```

原因是语言启用、编译器识别和标准库模块能力探测发生在 `project()` 期间。`project()` 之后再设置 gate，已经错过探测窗口。

随后要检查 CMake 探测结果：

```cmake
if(NOT CMAKE_CXX_COMPILER_IMPORT_STD)
  message(FATAL_ERROR "import std not enabled")
endif()
if(NOT 23 IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
  message(FATAL_ERROR "C++23 std module not available")
endif()
```

最后给具体 target 打开标准库模块：

```cmake
add_executable(i1_reference reference/main.cpp)
target_compile_features(i1_reference PRIVATE cxx_std_23)
set_property(TARGET i1_reference PROPERTY CXX_MODULE_STD ON)
set_property(TARGET i1_reference PROPERTY CXX_SCAN_FOR_MODULES ON)
```

顶层 `C01_Build_Compile_Link/exercises/CMakeLists.txt` 已把 gate 放在 `project()` 前，仅在 `ENGINEERING_STUDY_ENABLE_IMPORT_STD=ON` 时启用。普通课程构建不启用该实验能力；OFF 是“未请求”，不是通过或跳过。

## 证据链：源码、扫描、metadata、编译、运行

`import std` 成功不能只靠“程序输出 10”。一个程序可以 `#include <vector>` 后输出同样结果。验证要覆盖五层：

1. 源码确实写了 `import std;`，没有 include 冒充。
2. CMake target 设置了 `CXX_MODULE_STD ON`，没有只靠普通 header。
3. 扫描产物 `.ddi` 的 `requires` 中出现标准库模块依赖。
4. `CXXModules.json` 或等价 metadata 中出现 `std.ifc`、`std.compat.ifc`。
5. stdout 显示 `std.ixx`/`std.compat.ixx` 被编译，最终 exe 运行通过。

本机 probe 和 I1 验证保存了这些证据。`CXXModules.json` 证明 CMake 为 target 建立了标准库模块引用和 BMI/IFC metadata；`.ddi requires std` 证明 scanner 从实际 importer 源码看到了 `import std;`；最终运行只证明链接后的程序行为正确。证据要组合读取，单项不能独立覆盖全部风险。

## Direct `cl` 路径

Direct 路径显式编译 MSVC 自带标准库接口：

```cmd
cl /nologo /std:c++latest /EHsc /c /interface "D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\modules\std.ixx" /ifcOutput std.ifc /Fostd.obj
cl /nologo /std:c++latest /EHsc /c reference\main.cpp /reference std=std.ifc /Fomain.obj
link /nologo main.obj std.obj /out:import_std_direct.exe
import_std_direct.exe
```

这条路径没有 CMake scanner，也没有 Ninja dyndep。它适合排除“编译器或标准库本身不能用”的问题。若 direct 成功但 CMake 失败，下一步看 gate、`CMAKE_CXX_COMPILER_IMPORT_STD`、generator、dyndep 和 target property。

## CMake 路径

CMake 路径使用 Ninja 和 compiler scanner。典型输出会包含：

```text
Building CXX object ... std.ixx.obj
Building CXX object ... std.compat.ixx.obj
Building CXX object ... main.cpp.obj
Linking CXX executable i1_reference.exe
```

还应检查 build tree 中 target 的 module metadata。metadata 里出现 `std.ifc` 不是“consumer 没 include”的唯一证明；它证明 CMake 生成了标准库模块映射。必须再核对 `reference/main.cpp` 源码和 scanner `.ddi`，才能排除 include 冒充。

## 执行面边界

本机正常执行面已经验证 CMake 4.2.3 + VS Ninja 1.13.2 + MSVC 19.51 可以完成 named modules、`import std` 和 module package consumer。早期受限 runner 面曾在 CMake ABI try_compile 超时。最终课程只记录这个边界：正常执行面通过，受限面曾超时；不把未证实的底层原因写成结论。

`compiler working skipped` 也要读对。CMake 在 ABI 探测成功后，后续日志可能显示 working compiler check skipped。这是复用同一 configure 中已获得的证据，不是跳过能力检查，也不是强行设置 `CMAKE_CXX_COMPILER_WORKS`。

## 练习连接

`I1_import_std` 是独立练习。它既有 CMake target 路径，也有 `direct-cl/direct_import_std.cmd`。如果本机没有 CMake 4.2.3 gate 或没有 MSVC 标准库 `std.ixx`，练习应明确报告“工具未具备”；如果源码把 `import std;` 改成 include 后通过，checker 不应把它计为 `import std` 成功。

## 自测

- 为什么 gate 必须在 `project()` 前？
- Direct `cl` 路径成功能证明 CMake `CXX_MODULE_STD` 成功吗？
- `CXXModules.json` 里出现 `std.ifc` 能证明什么，不能单独证明什么？
- 为什么要同时看源码和 `.ddi requires std`？

答案：语言启用和编译器能力探测发生在 `project()`；不能，direct 和 CMake 是两层；metadata 证明 CMake 建立标准库模块映射，但单独不能证明源码没有 include 冒充；源码证明作者意图，`.ddi` 证明 scanner 对实际编译输入识别到了 `import std`。
