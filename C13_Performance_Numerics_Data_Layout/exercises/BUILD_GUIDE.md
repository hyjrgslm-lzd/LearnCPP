# C13 构建与运行

这是独立 CMake 工程。最低 CMake 3.28，C++23 标准库必须提供 `std::mdspan`；基线使用普通头文件，不要求 Modules。Windows 完整预设采用 MSVC x64、NMake 与 Release，离线核心预设采用 Ninja。进入 Visual Studio 的 x64 Native Tools Command Prompt，再切换到本目录。普通 PowerShell 不会自动获得 MSVC 的 INCLUDE/LIB 环境。

## 完整课程：包括第三方示例

```powershell
cmake --preset full-windows
cmake --build --preset full-windows
ctest --preset full-windows
```

第一次配置允许联网获取固定版本，源码放在对应 build 目录的 `_deps`。不会安装到系统目录，不要求 Conan/vcpkg，也不会运行依赖项目自己的测试套件。依赖来源和真实接口见 [标准与实现索引](../references/standards-and-implementations.md)。

`full-windows` 使用 NMake，避免某些受限进程环境下 Ninja 子进程完成通知停滞。若使用 `core` 且 PATH 中有多个 Ninja，可显式选用 Visual Studio 自带的版本，例如：

```powershell
cmake --preset core -DCMAKE_MAKE_PROGRAM=D:/VisualStudio2026/Installed/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe
```

路径是安装位置示例。若需要为离线核心切换生成器，使用新目录，例如 `cmake -S . -B ../build/core-nmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TRY_COMPILE_CONFIGURATION=Release`。不能在已有 Ninja 缓存中直接替换生成器。

`CMAKE_TRY_COMPILE_CONFIGURATION=Release` 让配置探针也使用所选配置，避免默认 Debug/PDB 路径引入另一套构建条件。预设中的 `VSLANG` 只影响子进程，不修改系统环境；实际诊断语言也取决于已安装的工具语言包。

已有固定源码可通过 `FETCHCONTENT_SOURCE_DIR_GSL_LITE`、`FETCHCONTENT_SOURCE_DIR_XSIMD`、`FETCHCONTENT_SOURCE_DIR_KOKKOS_MDSPAN`、`FETCHCONTENT_SOURCE_DIR_STDBLAS`、`FETCHCONTENT_SOURCE_DIR_MP_UNITS` 指向各库根目录，再加 `-DC13_FETCH_DEPS=OFF`。例如在一次成功准备依赖后，另一构建目录可复用这些源码；来源必须与依赖清单一致。若受限环境中的下载报 `schannel: AcquireCredentialsHandle failed: SEC_E_NO_CREDENTIALS`，复用已准备源码即可，不需要关闭 TLS 证书验证。

## 离线核心与学生作业

```powershell
cmake --preset core
cmake --build --preset core --parallel 4
ctest --preset core
cmake --preset student
cmake --build --preset student --parallel 4
```

`core` 关闭第三方与原生前沿，但保留普通数值、布局、矩阵、CPU 观察和综合项目。`student` 额外关闭 Reference/good/bad；不等于学生题已经做对。实现型 Student 初始会输出明确未完成信息并非零退出。观察型实验则是完整程序，其成功仅证明实际执行的检查通过。

各实现练习可独立配置；以 L02 为例，从本目录执行：

```powershell
cmake -S L02_layout -B ../build/layout -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TRY_COMPILE_CONFIGURATION=Release
cmake --build ../build/layout
../build/layout/c13_layout_student.exe
ctest --test-dir ../build/layout --output-on-failure
```

修改 `student/solution.hpp` 后重新编译并直接运行 student 目标。若希望由 CTest 登记 Student，配置时加 `-DC13_TEST_STUDENTS=ON`；未完成题目将真实失败，不会被翻译为跳过。

## 什么算构建成功

`Configuring done` 和 `Generating done` 只表示构建工程生成。必须继续 `cmake --build`，看到各目标实际链接，才算程序编译通过。默认构建包含 Student 的编译，但默认 CTest 不运行未完成 Student；Reference、独立 good、预期被拒绝的 bad 含义不同。

短检查使用 Release 中仍有效的 `check`。故意错误的 bad 需要退出码 1 和检查器诊断同时满足；异常崩溃、超时、程序不存在不能冒充反例成功。

原生前沿缺少匹配头文件或真实接口时，配置会明确记录不可用。不能把第三方 stdBLAS/xsimd 成功视为本机原生 `<linalg>`/`<simd>` 成功。NUMA 的跨节点放置实验需要实际拓扑和权限，普通单机核心编译不代表完成了多节点性能验证。

## 本课验收边界

只需本机 Release 构建和课程短检查。性能驱动与硬件专题保留完整入口，但默认不运行长测、NUMA放置、全平台矩阵、Sanitizer 或上游测试。若要形成性能排名，另按实验章节采样，避免把本次轻量验收日志当成完整性能研究。

所有构建、下载、日志、CSV 样本与审查记录留在 `build/` 等忽略目录。教材仅记录复现方法与适用条件，不链接某次机器私有报告。
