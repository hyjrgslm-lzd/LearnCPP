# C01：构建、编译链接与工具链

这门课面向已经会写 C++、希望弄清程序怎样被构建、装载、调试和交付的工程师。起点不是一份复杂 CMake 配置，而是一个能运行的小程序：先观察源码经过哪些阶段，再解释每个阶段接受什么输入、产生什么文件、在什么条件下失败。

学完后，你应能定位跨翻译单元的定义问题，检查对象文件和动态库的符号，解释公共接口及运行库兼容边界，写出可被另一项目消费的 CMake 包，并分别验证 named modules 和 `import std` 的真实工具链要求。教程里的小库是观察这些机制的载体，不是一个生产插件框架。

## 阅读路线

| 顺序 | 正文 | 完成后能解释与操作什么 |
|---|---|---|
| 00 | [构建与调试](chapters/00-build-and-debug.md) | 从源文件到可执行文件，断点、变量、调用栈与优化影响 |
| 01 | [翻译与预处理](chapters/01-translation-and-preprocessing.md) | 为什么每个 TU 有自己的包含结果和宏状态 |
| 02 | [ODR 与定义](chapters/02-odr-and-symbols.md) | include guard 为什么不解决跨 TU 重复定义，怎样选择单定义或 inline |
| 03 | [对象、符号与静态库](chapters/03-object-files-and-static-libraries.md) | 由未解析符号追到对象、归档与链接命令 |
| 04 | [动态库与运行时](chapters/04-dynamic-libraries-and-runtime.md) | 导入库、导出、装载、CRT、初始化与退出 |
| 05 | [ABI 与接口](chapters/05-abi-boundaries.md) | 哪些变化破坏二进制边界，谁分配谁释放 |
| 06 | [CMake 与依赖](chapters/06-cmake-and-dependencies.md) | target 使用要求如何传递，怎样固定和消费依赖 |
| 07 | [诊断与构建成本](chapters/07-diagnostics-and-build-cost.md) | 选择正确诊断工具，区分编译/链接/运行问题并定位构建成本 |
| 08 | [Modules](chapters/08-modules.md) | 接口、分区、扫描、BMI 与构建依赖顺序 |
| 09 | [import std](chapters/09-import-std.md) | 库模块与语言 Modules 的差异，怎样验证当前组合 |
| 10 | [打包与兼容性](chapters/10-packaging-and-compatibility.md) | 安装、导出、版本检查、重定位及独立 consumer |

00 只要求基础函数、变量与控制流。01—03 连续建立翻译与链接模型；04—05 在其后进入装载和二进制边界。06 可以在03之后开始，07在00与06之后进入；08需要01—03及06，09再接08。10组合03—06；named-module包消费在08之后进入。更深入的模板推导/查找归C04，对象模型归C02；本课先讲足当前工程实验实际需要的规则。

## 练习与证据

先读正文，写下对结果的预测，再运行观察程序或编辑 Student，最后查看完整解析。`_student` 与 `_reference` 是独立程序；实现型学生入口未完成时应明确失败。观察程序运行成功只说明它执行过的观察，不代表你已经完成预测、解释和扩展任务。

构建入口见 [BUILD_GUIDE](exercises/BUILD_GUIDE.md)。核心默认离线、C++23；Modules、`import std` 和故意错误诊断有独立入口。每次错误都先说明期望在哪个阶段失败；超时、错误阶段不符或任意别的报错不能算负例验证通过。

本次实现与验收约定见[实施规格](references/implementation-spec.md)。11章与13个练习单元已实现，Windows四配置与专项实验已完成验证；知识/练习对应、原始输出和非作者审查分别从[覆盖表](references/coverage.md)、[质量报告](references/quality-report.md)进入。G2另保留72轮正式测量及其适用边界。观察型实验成功不等于完成学习任务，未完成Student仍需自行实现。

## 接入其他课程

- [Concurrency](../C08_Concurrency/README.md)主讲同步、发布与回收；本课为其 CMake 配置、编译器/库能力探测、Release 检查和诊断提供工程先修。
- [Coroutine](../C09_Coroutines/README.md)主讲协议与生命周期；本课的 ABI、符号和优化观察衔接 J2/F3，依赖管理衔接真实 I/O 与 sender 桥接。
- [全局目标计划](../LEARNCPP_GLOBAL_PLAN.md)决定18个课程族的最终去向；本课交付不能替代其他课程族。Windows 实测与其他平台可复現规格分别记录，不能相互冒充。

## C02 对象与资源先修

语言层的初始化、部分构造失败与对象表示见 [C02](../C02_Objects_Lifetime_Ownership/README.md) 的[构造与展开](../C02_Objects_Lifetime_Ownership/chapters/04-construction-and-unwinding.md)及[布局与表示](../C02_Objects_Lifetime_Ownership/chapters/11-layout-and-representation.md)。这些规则支撑本课的 ABI 和跨模块资源责任；具体符号、运行库、装载与二进制兼容仍由本课主讲。

## C04 泛型与编译期桥接

[进入C04课程](../C04_Generic_CompileTime_Reflection/README.md)。模板实例化的语义、查找、类型计算与定制点由C04主讲；本课继续负责翻译、链接、ABI及构建系统。编译成本实验可在掌握两侧当前先修后互相回访。
