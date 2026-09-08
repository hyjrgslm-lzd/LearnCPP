# C01 知识、练习与下游能力

这张表登记实际教学责任，完成状态与非作者证据统一见[质量报告](quality-report.md)。阅读正文、运行观察、完成实现题是不同的动作；对应练习 README 给出逐 Part 作业、独立编辑位置和解析。没有因建立本表而将尚未审查的单元标成完成。

| 单元 | 正文与知识 | 关键能力/检查 |
|---|---|---|
| [A1_build_debug](../exercises/A1_build_debug/README.md) | [00](../chapters/00-build-and-debug.md)：编译、运行和调试 | 在自己的调用链上断点、单步、读已初始化变量和栈；Debug与优化观察分开 |
| [B1_preprocessor](../exercises/B1_preprocessor/README.md) | [01](../chapters/01-translation-and-preprocessing.md)：翻译单元、宏和包含 | 对照两个TU的预处理文本与宏状态，解释声明定义及include guard |
| [C1_odr](../exercises/C1_odr/README.md) | [02](../chapters/02-odr-and-symbols.md)：ODR、linkage和inline | 单消费者基线、两TU重复符号、预处理/符号定位、两种独立修复及学生路径 |
| [C2_archive](../exercises/C2_archive/README.md) | [03](../chapters/03-object-files-and-static-libraries.md)：对象与静态库 | 从未解析符号定位所缺定义/归档成员；区分COFF与ELF的工具行为 |
| [D1_shared_library](../exercises/D1_shared_library/README.md) | [04](../chapters/04-dynamic-libraries-and-runtime.md)：动态库与运行时 | static/import library/DLL、显式加载、缺导出/缺DLL、初始化和结束观察 |
| [E1_abi](../exercises/E1_abi/README.md) | [05](../chapters/05-abi-boundaries.md)：ABI与所有权 | C consumer、opaque handle、创建销毁、版本/空参数/范围拒绝、异常转换 |
| [F1_cmake_targets](../exercises/F1_cmake_targets/README.md) | [06](../chapters/06-cmake-and-dependencies.md)：target使用要求 | 头目录、定义、语言要求与链接依赖的PUBLIC/PRIVATE/INTERFACE传递 |
| [F2_dependencies](../exercises/F2_dependencies/README.md) | [06](../chapters/06-cmake-and-dependencies.md)：固定依赖 | 本地来源、安装包、find_package/FetchContent与版本边界，默认离线 |
| [G1_diagnostics](../exercises/G1_diagnostics/README.md) | [07](../chapters/07-diagnostics-and-build-cost.md)：诊断与检查 | Release有效检查、静态分析、ASan正反例、有限fuzz、诊断与缺工具状态区分 |
| [G2_build_cost](../exercises/G2_build_cost/README.md) | [07](../chapters/07-diagnostics-and-build-cost.md)：构建成本 | clean/no-op/实现/公共头输入；依赖与阶段定位先于PCH/LTO比较 |
| [H1_modules](../exercises/H1_modules/README.md) | [08](../chapters/08-modules.md)：语言Modules | interface/implementation/partition、fragment、BMI依赖扫描和模块包消费 |
| [I1_import_std](../exercises/I1_import_std/README.md) | [09](../chapters/09-import-std.md)：标准库模块 | gate、metadata、标准级别、target属性、实际编译链接运行；不以include冒充 |
| [J1_package](../exercises/J1_package/README.md) | [10](../chapters/10-packaging-and-compatibility.md)：交付兼容 | 独立consumer、static/shared、Debug/Release、复制prefix重定位及失败对照 |

## 从下游反向检查深度

1. **Concurrency 的 verify-core 为什么不等于完整库能力？** A1/F1/F2/G1必须使读者能区分配置、实例化、链接、运行、未请求和真实SKIP，再进入并发课的native probes。原并发算法和性能正文保留在[C08主课](../../Concurrency_Study/README.md)。
2. **Coroutine J2为什么不应跨模块裸传协程帧？** C2/D1/E1必须先解释符号只解决名字、ABI还约束布局与运行时、对象销毁属于哪个模块，然后再看协程额外的帧存活责任。具体帧协议仍由[C09](../../Coroutine_Study/README.md)主讲。
3. **Coroutine F3的编译结果能证明什么？** A1/G1/G2必须区分特定输入的生成代码、优化报告与普遍保证；确认配置、编译器及实际参数，再解释是否观察到分配消除。一次结果不能代替语言保证。
4. **独立项目怎样消费一个库？** C2/D1/E1/F1/F2/J1连成一条链：需要的定义在哪里、公开接口是什么、target传递什么、装载器怎样找库、安装后的consumer如何得到同一契约。H1模块导出是其后独立分支，不能把它的BMI当成普通header包。

这些反向路线必须经非作者遮答案试做，检查器只证明具体运行条件，不替代教学审查。C01不代替C02完整对象模型、C04模板进阶、C07系统I/O、C13完整性能数值课程或C18生产插件/语言绑定。
