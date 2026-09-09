# 严格判分与 fast 内核的多源接线

fast 专项已由主线程接入 [runtime_tests/CMakeLists.txt](../../exercises/runtime_tests/CMakeLists.txt)，由 `CONCURRENCY_STUDY_TEST_FAST_MATH` 控制；核心默认关闭，完整 Windows preset 开启。专题作者未修改公共 CMake。普通 runtime_numeric_test 保持单源并禁止 fast 模式；专项使用本目录两个独立 `.cpp`，不能让 `*_test.cpp` glob 把它们各自当作单源测试。

| 单元 | 内容 | 本机编译参数 |
|---|---|---|
| [fast_math_check.cpp](fast_math_check.cpp) | 输入、解析/补偿 oracle、误差界、比较、故障检查、main | `/O2 /fp:strict /GL-` |
| [fast_math_kernel.cpp](fast_math_kernel.cpp) | 包装真实 add/dot/GEMM 内核，无 oracle | `/O2 /fp:fast /GL-` |
| [fast_math_api.hpp](fast_math_api.hpp) | 枚举和函数声明，无 inline 算术 | 仅作为两侧 ABI |

最终链接 `/LTCG:OFF /OPT:NOICF`。两个源都使用 C++23 和现有 include/能力宏；内核侧使用已有 xsimd 可选依赖，不需要新链接库。当前 MSVC 原生 SIMD 能力缺失，专项没有声称验证该分支。

实际工程将内核放在独立 object target，并链接到严格检查目标，避免严格/fast 选项互相传递。下面仅展示接线原理，路径以 exercises 根目录为参照；真实目标及配置级 IPO 处理以链接的 CMake 为准：

```cmake
add_library(numeric_fast_kernel OBJECT ../topics/simd/fast_math_kernel.cpp)
cs_configure_target(numeric_fast_kernel)
add_executable(numeric_fast_check ../topics/simd/fast_math_check.cpp
    $<TARGET_OBJECTS:numeric_fast_kernel>)
cs_configure_target(numeric_fast_check)
set_target_properties(numeric_fast_kernel numeric_fast_check
    PROPERTIES INTERPROCEDURAL_OPTIMIZATION FALSE)
if(MSVC)
    target_compile_options(numeric_fast_kernel PRIVATE /fp:fast /GL-)
    target_compile_options(numeric_fast_check PRIVATE /fp:strict /GL-)
    target_link_options(numeric_fast_check PRIVATE /LTCG:OFF /OPT:NOICF)
else()
    target_compile_options(numeric_fast_kernel PRIVATE -ffast-math -fno-lto)
    target_compile_options(numeric_fast_check PRIVATE -fno-fast-math -ffp-contract=off -fno-lto)
    target_link_options(numeric_fast_check PRIVATE -fno-lto)
endif()
```

numeric_fast_check 已注册为 30 秒超时的专项 CTest。配置级 IPO（例如 `INTERPROCEDURAL_OPTIMIZATION_RELEASE`）或全局 flags 也不能覆盖禁用约定；应看实际编译/链接命令，不能只看一个 CMake 属性。工程显式关闭一般和当前配置级 IPO；GCC/Clang 链接额外使用 `-fno-fast-math -fno-lto`，避免引入进程级浮点环境修改。跨工具链选项仍须分别验证，本次实际构建和运行使用 MSVC 19.51，成功且无编译告警。

统一入口从 exercises 执行 `cmake --preset full-windows`、`cmake --build --preset full-windows --target numeric_fast_check`，再执行 `ctest --test-dir build/full-windows -C Release -R '^numeric_fast_check$' --output-on-failure`。主线程实测日志是本机归档 `exercises/build/full-windows/fast-build.log`；实际 kernel 为 `/fp:fast /GL-`，checker 为 `/fp:strict /GL-`，链接为 `/LTCG:OFF /OPT:NOICF`。

在本机不修改 CMake 即可复现：

```powershell
# 工作目录 C08_Concurrency
./topics/performance/verify-numeric.ps1 -FastMath
./topics/performance/verify-numeric.ps1 -FastMath -AddressSanitizer
```

启用可选库时追加 `-XsimdInclude` 的真实 include 路径。新产物位于 `exercises/build/numeric-review-p2-fast-isolated*`，保存 verification.log 和两个单元的汇编。脚本绝不会以 fast 参数编译整个 numeric_test；严格检查器和 fast 内核都有编译模式守卫，错误配置必须产生编译失败。实际复验已把两个模式故意交换，分别确认守卫报错。

不共享生产 inline 定义还有另一个目的：如果严格 oracle 单元也 include 数值内核头，即使两侧禁止 LTO，链接器也可能选择具有相同外部名称的 inline/COMDAT 实体，令“到底测了哪个浮点版本”变得含糊。本专项的严格单元只读 API 声明和独立 oracle，不制造这个歧义；普通检查工具中的 cs::check 不含浮点算术。

依据：[MSVC /fp](https://learn.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior?view=msvc-170)、[MSVC /GL](https://learn.microsoft.com/en-us/cpp/build/reference/gl-whole-program-optimization?view=msvc-170)、[GCC 优化选项](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)、[Clang 浮点模型](https://clang.llvm.org/docs/UsersManual.html#floating-point-model)。编译边界由实际选项和生成对象共同验证，不把 fast 模式下的补偿式当作可靠判分器。
