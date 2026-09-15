# L09 dynamic loading

目标：实现 `c07_l09::load_module_add()`，按绝对路径加载 CMake 构建出的共享库，查找 `extern "C"` 导出符号，调用 `uint32_t` ABI 函数，并在返回前释放本次打开的模块 handle。

## Part

1. 加载 `$<TARGET_FILE:L09_dynamic_loading_module>` 的绝对路径。
2. 查找 `c07_module_mix`，用 checker 传入的 runtime nonce 调用并返回无符号 32 位结果。
3. 缺失符号必须受控失败。
4. 缺失库必须受控失败。
5. 相对路径必须拒绝；本题不修改全局搜索路径。

## 接口

```cpp
c07_l09::ModuleResult load_module_add(
    const std::filesystem::path& library,
    std::string_view symbol,
    std::uint32_t a,
    std::uint32_t b,
    std::uint32_t nonce);
```

checker 持有真实 DLL/SO fixture 的 observer handle，并读取模块导出的调用计数。实现必须实际调用模块导出的 C ABI 函数；只复算返回值或自报 `unloaded=true` 会被调用计数拒绝。本题验证“本次 loader 没泄漏可见函数指针并释放自身 handle”的代码路径，不把 observer 仍持有模块期间的全局卸载状态当作可证明事实。

## 本地命令

```sh
cmake --build C07_OS_Memory_System_IO/exercises/build/process-author-debug --config Debug --target L09_dynamic_loading_reference
ctest --test-dir C07_OS_Memory_System_IO/exercises/build/process-author-debug -C Debug -R L09_dynamic_loading
```
## IDE 项目

生成 Visual Studio 工程后，启动项目是 `L09_dynamic_loading_student`。

本题是实现题。学习者只改 Student 入口；Reference、validation 和 checks 只用于对照与验证。
- Student 入口：`src/student/dynamic_loading.hpp`。
- Checker 入口：`checks/dynamic_loading_checks.cpp`、`checks/include/dynamic_loading_contract.hpp`、`checks/module.cpp`。
- Reference 对照：`src/reference/dynamic_loading.hpp`。
- validation/good 对照：`validation/good/dynamic_loading.hpp`。
- validation/bad 反例：`validation/bad/dynamic_loading.hpp`。

单题独立构建：从本目录运行 cmake -S . -B build/leaf -G "Visual Studio 18 2026" -A x64，然后 cmake --build build/leaf --config Debug --target L09_dynamic_loading_student。
修改后先重建 `L09_dynamic_loading_student`，再按课程 `BUILD_GUIDE.md` 运行对应检查。
