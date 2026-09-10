# C02：对象模型、生命周期与所有权

一段代码拿到了地址，不等于那里已经存在可访问的对象；拿到了引用，也不等于引用能让对象继续活着。这门课从这些最常见、也最容易混用的判断开始，建立初始化、表达式、对象、存储、生命周期和资源责任之间的关系。

读者应已经能编写基本的现代 C++ 程序，使用函数、类与标准容器，并能按 [C01](../C01_Build_Compile_Link/README.md) 构建和调试。本课不要求先学完模板、协程或内存模型。遇到教学实现需要的模板、异常和分配接口，会先讲足当前问题需要的规则；完整泛型与分配器系列由后续课程承担。

学完后，你应能根据标准语义判断对象何时存在、引用何时有效，设计并实现资源的获得、转交与清理，在失败路径中恢复容器自身状态，并说明编译器输出和诊断工具各自能证明什么。正文是主线：先解释结果，再运行观察、补全 Student，最后对照完整解析。

## 阅读路线

| 章 | 正文 | 要解决的问题 |
|---|---|---|
| 00 | [模型与路线](chapters/00-model-and-route.md) | 类型、对象、存储和资源分别是什么 |
| 01 | [初始化](chapters/01-initialization.md) | 不同初始化语法实际建立什么状态 |
| 02 | [表达式与引用](chapters/02-expressions-and-references.md) | 值类别怎样影响绑定和调用 |
| 03 | [生命周期与借用](chapters/03-lifetime-and-borrowing.md) | 临时对象、闭包和 view 何时失去依托 |
| 04 | [构造与展开](chapters/04-construction-and-unwinding.md) | 部分构造失败时谁负责析构 |
| 05 | [特殊成员](chapters/05-special-members.md) | 复制、移动和隐式生成怎样影响资源 |
| 06 | [移动与返回](chapters/06-move-and-return.md) | 转型、实际移动和消除之间是什么关系 |
| 07 | [RAII 与所有权](chapters/07-raii-and-ownership.md) | 怎样把正常和失败路径的责任放进对象 |
| 08 | [独占所有权](chapters/08-unique-ownership.md) | 怎样正确使用独占指针和自定义删除器 |
| 09 | [共享所有权](chapters/09-shared-ownership.md) | 对象、别名指针和控制块何时存活 |
| 10 | [控制块实现](chapters/10-control-block.md) | 强弱计数如何分别管理两种生命期 |
| 11 | [布局与表示](chapters/11-layout-and-representation.md) | 地址、对齐、对象表示允许推导什么 |
| 12 | [存储与对象创建](chapters/12-storage-and-object-creation.md) | 获取存储后怎样合法建立和结束对象 |
| 13 | [别名与指针来源](chapters/13-aliasing-and-provenance.md) | 转型为何不自动赋予访问权 |
| 14 | [UB 与优化](chapters/14-undefined-behavior-and-optimization.md) | 规范、优化观察和诊断为何需要分开 |
| 15 | [资源容器](chapters/15-object-buffer.md) | 怎样组合构造、迁移、失败回滚和借用 |

00—06 建立语义底座；07—10 深入资源管理；11—14 在前述对象模型上进入存储与访问边界；15 综合使用这些能力。每章另列实际硬先修，不能用编号顺序代替能力检查。源码阅读随独占指针、控制块、显式生命期和容器迁移一起进行。

## 实验、学习完成与证据

构建和题型入口见 [BUILD_GUIDE](exercises/BUILD_GUIDE.md)。实现型 Student 与 Reference 是独立目标；未完成 Student 的正确初始结果是明确失败。观察实验运行成功只证明已经检查的观察，不代替读者的预测、解释和扩展任务。

标准主线固定 C++23，C++26/29 草案与 DR 单独登记规范/实现状态。真实未定义行为默认不运行；安全模型与实际诊断结果分别解释。核心为本机离线验证，缺少工具或语言能力时保留完整材料并说明未验证边界。

本课已按[实施规格](references/implementation-spec.md)完成C02 Windows范围交付及独立审查。[覆盖表](references/coverage.md)连接正文与练习，[质量报告](references/quality-report.md)记录实际验证、能力跳过、未测平台及最终审查依据。

## 下游衔接

- [C03 类型建模与接口设计](../C03_Type_Modeling_Interface_Design/README.md)：从对象合法、资源有归属，继续进入类不变量、独立值、错误通道、多态与接口演进；[批事务](../C03_Type_Modeling_Interface_Design/chapters/06-exception-safety-and-transactions.md)把异常展开用于可观察状态保证。
- [Coroutine](../C09_Coroutines/README.md)：把对象/借用基础用于闭包、协程帧和挂起后的存活责任。
- [Ranges](../C06_Ranges/README.md)：把借用和失效基础用于 view/iterator；borrowed_range 不为底层 owner 保活。
- [Execution](../C10_Execution/README.md)：把资源归属用于 operation state、receiver 与完成后的收束。
- [Concurrency](../C08_Concurrency/README.md)：在对象生命期上另外建立同步、发布和回收协议；引用计数不自动保证对象访问同步。

全局覆盖及 C03/C04/C06/C07 的职责见[全局计划](../LEARNCPP_GLOBAL_PLAN.md)。本课只承担 C02，领域协议和完整并发保证留在对应主课。

## C04 泛型与编译期桥接

[进入C04课程](../C04_Generic_CompileTime_Reflection/README.md)。C04展开推导、引用折叠、转发、泛型约束和常量求值。对象存活、借用与存储仍由本课主讲；模板包装能保留值类别，不会自动延长对象生命期。
