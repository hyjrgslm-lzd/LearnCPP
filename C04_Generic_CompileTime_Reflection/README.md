# C04：泛型、编译期编程与反射

2026-09-10 修订增量已完成，现有25章/21单元。当前结果见[增量质量报告](references/revision-quality-report-20260910.md)与[交付清单](references/revision-delivery-manifest.md)；前次18章/14单元的验证保留为历史基线。

本课把“能写一个模板”推进到“能解释查找、推导、约束、实例化和常量求值发生在哪一步，并据此设计可诊断的泛型接口”。正文负责推导，练习用真实类型、调用和编译器输出验证理解。现包含25章与21个练习/实验单元；[质量报告](references/quality-report.md)区分各批验证、独立审查和前沿未测范围。

入口需要[C01](../C01_Build_Compile_Link/README.md)基本构建能力和[C02](../C02_Objects_Lifetime_Ownership/README.md)对象、引用与生命周期基础；错误通道在综合项目前补读[C03](../C03_Type_Modeling_Interface_Design/README.md)。没有学过Ranges、sender或协程也可以开始；它们是本课机制的下游应用。

## 阅读顺序

先读00路线；01—06建立声明、推导、转发、查找、重载和约束的连续模型；07—10形成类型计算与常量算法能力；11把这些规则用于定制点。12讨论编译器付出的成本；13—15建立真实静态反射模型；16集成字段工具；17沿真实源码和下游题目检查理解。

18—24进入进阶：18的值计算连接19的字段DSL，21的类型算法连接23的Mp11；20与22分别深化对象转发和表达式保存；24用Hana对照编译期键与运行期值。它们按每章先修选读。完成相应机制后，可继续[C05 fmt](../C05_Data_Representation_Standard_Facilities/chapters/19-fmt-library.md)和[spdlog前端](../C05_Data_Representation_Standard_Facilities/chapters/20-spdlog-frontend.md)；这条扩展路线不改变C05原基础入口的先修。

每章的硬先修按实际知识写明。样章11在建设时包含局部先修讲解，读完整主线后可把这些段落当作机制回顾，不意味着其余主题可以略去。

| 章 | 主讲入口 |
|---|---|
| 00 | [学习路线与证据](chapters/00-learning-route.md) |
| 01 | [模板声明、实例化、特化与ODR](chapters/01-template-model.md) |
| 02 | [推导、auto与decltype](chapters/02-deduction.md) |
| 03 | [转发、初始化与CTAD](chapters/03-forwarding-ctad.md) |
| 04 | [查找、依赖名与ADL](chapters/04-lookup-adl.md) |
| 05 | [重载、偏序与SFINAE](chapters/05-overload-sfinae.md) |
| 06 | [约束、归一化与语义公理](chapters/06-constraints.md) |
| 07 | [参数包、折叠与NTTP](chapters/07-packs-nttp.md) |
| 08 | [类型列表与签名计算](chapters/08-type-lists.md) |
| 09 | [tuple与异构遍历](chapters/09-tuple-traversal.md) |
| 10 | [常量求值与存储边界](chapters/10-constant-evaluation.md) |
| 11 | [定制点与表达式契约](chapters/11-customization-points.md) |
| 12 | [编译诊断与成本实验](chapters/12-compile-cost.md) |
| 13 | [反射模型与查询](chapters/13-reflection-model.md) |
| 14 | [splicing、展开与生成](chapters/14-splicing-generation.md) |
| 15 | [annotations及前沿版本](chapters/15-annotations-frontier.md) |
| 16 | [静态字段综合项目](chapters/16-static-record.md) |
| 17 | [源码与下游回访](chapters/17-source-and-bridges.md) |
| 18 | [值级编译期算法与静态结果](chapters/18-compiletime-values.md) |
| 19 | [字段选择与投影DSL](chapters/19-field-projection-dsl.md) |
| 20 | [显式对象参数与forward_like](chapters/20-explicit-object-forwarding.md) |
| 21 | [类型管线、组合与完成签名](chapters/21-type-pipelines.md) |
| 22 | [表达式模板、拥有与别名](chapters/22-expression-templates.md) |
| 23 | [Boost.Mp11与元map](chapters/23-mp11.md) |
| 24 | [Boost.Hana异构计算对照](chapters/24-hana.md) |

## 构建与证据

使用[构建指南](references/BUILD_GUIDE.md)。实现型Student有意未完成；正常核心验证运行Reference、观察程序和有效的检查器正反控制，不把运行Reference当成学生已完成。

新增实现题为[A01](exercises/A01_compiletime_values/README.md)、[A02](exercises/A02_field_projection/README.md)、[A03](exercises/A03_explicit_object/README.md)、[A04](exercises/A04_type_pipelines/README.md)、[A05](exercises/A05_expression_templates/README.md)与[U01 Mp11](exercises/U01_mp11/README.md)；[U02 Hana](exercises/U02_hana/README.md)是观察/迁移题。第三方库使用固定版本和显式准备的`meta-libs`预设；默认核心保持离线。

[逐文件交付清单](references/revision-delivery-manifest.md)记录源码、文档与证据指纹；[规范与实现索引](references/standards-and-implementations.md)分别记录语言规范和实际能力。本机验证是Windows可用路径；反射等前沿的源码存在或探测SKIP不代表其运行通过。

## 主讲与桥接

- C06讲range/iterator/view协议，本课讲其实际依赖的cvref、约束、ADL和类型计算。
- C09讲协程控制与帧生命周期，本课讲协议选择涉及的查找、重载和转发。
- C10讲完成通道与执行协议，本课讲类型集合、CPO与异常规格推导；教学CPO不冒充标准或某版stdexec的全部定制规则。
- C14讲设备与布局的具体含义，本课讲NTTP、traits、特化和编译成本。
- C15讲UHT、UObject及GC，本课讲语言静态反射；语言元信息不会自动产生引擎的运行时注册与对象追踪。
