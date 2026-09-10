# F01 frontier capability probes

本目录是C04第13-15章的观察实验。它不实现学生答案，不用traits伪装反射，也不把一个工具链的缺口推广成标准结论。

默认`GENERIC_STUDY_ENABLE_FRONTIER=OFF`时不注册任何test，状态记为OFF。打开frontier预设后，每个probe独立编译、链接、运行，并打印`header/macro/body`状态：

- exit 0：该项源码在当前工具链真实通过；
- exit 77：该项缺少明确feature macro、头文件或实现入口，CTest记为SKIP；
- 其他非0：工具链宣称支持但真实源码失败，记为FAIL。

本机已知MSVC 19.51没有`<meta>`，所以反射、splicing、annotations和`define_static_*`相关项预计SKIP；这不是pack indexing、fold constraints或constexpr exceptions的结论。

N5050表22的相关宏值：`__cpp_impl_reflection 202603L`、`__cpp_expansion_statements 202506L`、C++26类型/值包`__cpp_pack_indexing 202311L`、`__cpp_constexpr 202406L`、`__cpp_constexpr_exceptions 202411L`。N5054把`__cpp_concepts`提升到`202606L`，并把包含模板名包索引后的`__cpp_pack_indexing`提升到`202606L`。P2747R2另有库宏`__cpp_lib_constexpr_new 202406L`。`__cpp_fold_expressions`仍是`201603L`，所以fold expanded constraints用CMake真实编译探测记录body是否可用。P4101R1只写明提升`__cpp_consteval`到占位值，本课把`__cpp_consteval >= 202606L`作为当前实现观察入口，不当作标准常量。P3385R8只给`__cpp_impl_reflection_attributes 2026XXL`占位宏；probe只检测宏名，不编造数值。C++29模板名pack indexing和conditional `noexcept`复合要求单独用C29/最新模式探测；本机MSVC只有`/std:c++latest`入口时，未通过即明确SKIP。

F01的CMake只用最小正例决定是否开启无官方宏或实现可能滞后的主体：annotation正例只检查`<meta>`和`[[=1]]`语法；fold constraints、constexpr placement new、C++29模板名pack indexing和conditional `noexcept`复合要求用独立小程序检查行为。主体源码一旦启用，编译、链接或运行失败就是FAIL，不会回退成SKIP。

`constexpr_placement_new_probe.cpp`保留`C04_P2747_NEGATIVE_WRONG_STORAGE`负例入口，用来检查“把`int`构造到`unsigned char`存储并读回”不满足P2747R2的常量求值边界。当前MSVC正例能力都未通过，所以该负例不能被本机实测为标准拒绝，只能保留源码入口和force控制证据。`c29_conditional_noexcept_requirement_probe.cpp`保留`C04_P3822_NEGATIVE_NON_BOOL_CONDITION`入口，用来检查`noexcept(...)`里的条件不能转换为`bool`时复合要求不满足。

`GENERIC_STUDY_FRONTIER_ENABLE_FAILURE_CONTROL=ON`会额外注册`F01_declared_failure_control`。它不用前沿语法，只打印`macro=1 body=1`后返回1，用来验证已声明能力进入主体后的失败路径会被CTest报告为FAIL。默认关闭，避免影响正常12项能力采样。

规范入口：

- P2996R13 Reflection for C++26: https://wg21.link/p2996r13
- P3394R4 Annotations for Reflection: https://wg21.link/p3394r4
- P1306R5 Expansion Statements: https://wg21.link/p1306r5
- P3491R3 define_static_{string,object,array}: https://wg21.link/p3491r3
- P2662R3 Pack Indexing: https://wg21.link/p2662r3
- P2963R3 Fold expanded constraints: https://wg21.link/p2963r3
- P3068R6 Constexpr exceptions: https://wg21.link/p3068r6
- P2747R2 Constexpr placement new: https://wg21.link/p2747r2
- P3822R2 Conditional noexcept specifiers in compound requirements: https://wg21.link/p3822r2
- P3670R4 Template-name pack indexing: https://wg21.link/p3670r4
- P4101R1 Consteval-only values: https://wg21.link/p4101r1
- P3385R8 Attributes reflection proposal: https://wg21.link/p3385r8
