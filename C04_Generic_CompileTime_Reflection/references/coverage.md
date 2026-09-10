# C04 知识覆盖与下游反向检查

本表按全局计划4.2组织，不以文件数证明教学完成。状态以[质量报告](quality-report.md)和非作者记录为准；当前修订中的正文不能沿用旧版本批准。课程正文、练习入口和源码指纹共同定位证据。

| 核心知识与问题 | 主讲与代码入口 | 下游承接 |
|---|---|---|
| 模板声明与实例化、函数/类/变量/别名模板、按需实例化、全/偏特化、显式实例化和ODR | [01](../chapters/01-template-model.md)、[L01](../exercises/L01_templates/README.md)；成本分支[B01](../exercises/B01_compile_cost/README.md) | 模板库多TU组织，C01编译链接 |
| 值/引用/cv推导、退化、非推导上下文、auto/decltype/decltype(auto) | [02](../chapters/02-deduction.md)、[L02](../exercises/L02_deduction/README.md) | view/proxy返回表达式，协程包装 |
| 引用折叠、转发引用、move/forward、CTAD与初始化、返回与异常规格 | [03](../chapters/03-forwarding-ctad.md)、[L03](../exercises/L03_forwarding/README.md) | callable、tuple、sender变换 |
| 依赖名/两阶段查找、typename/template、ADL/hidden friend、访问和定制边界 | [04](../chapters/04-lookup-adl.md)、[L04](../exercises/L04_lookup/README.md)、[L11](../exercises/L11_customization/README.md) | C06定制、C09 co_await候选 |
| 候选、转换、函数模板偏序、类模板偏特化、SFINAE立即上下文与硬错误 | [05](../chapters/05-overload-sfinae.md)、[L05](../exercises/L05_overload/README.md) | 库接口选择与有限诊断 |
| requires四类、Concepts、归一化/原子身份/包含关系、约束偏序与语义公理 | [06](../chapters/06-constraints.md)、[L06](../exercises/L06_constraints/README.md) | range多遍历语义、CPO/生成接口支持域 |
| packs/fold/空包/顺序、NTTP结构化类型与模板模板参数 | [07](../chapters/07-packs-nttp.md)、[L07](../exercises/L07_packs/README.md) | 类型级配置、GPU布局参数 |
| traits、类型列表map/filter/concat/unique、惰性实例化、结果类型与签名通道 | [08](../chapters/08-type-lists.md)、[L08](../exercises/L08_type_lists/README.md) | C10 completion signatures，反射生成 |
| tuple/index_sequence/apply、cvref与callback责任、空集合与异常停止 | [09](../chapters/09-tuple-traversal.md)、[L09](../exercises/L09_tuple/README.md) | P1字段访问与异构数据 |
| constexpr/consteval、立即调用上下文、is_constant_evaluated/if consteval、对象/存储、暂存容器与算法 | [10](../chapters/10-constant-evaluation.md)、[L10](../exercises/L10_constexpr/README.md) | 编译期解析、反射meta暂存/静态生成 |
| invoke/CPO、member/ADL优先、cvref/noexcept与约束一致、安全反例 | [11](../chapters/11-customization-points.md)、[L11](../exercises/L11_customization/README.md) | Ranges与Execution具体协议对照 |
| 编译诊断、实例化时间、trace、符号/节大小、多TU显式实例化、原始样本与代价 | [12](../chapters/12-compile-cost.md)、[B01](../exercises/B01_compile_cost/README.md) | C01/C13工程测量，GPU泛型编译 |
| meta实体/对象、查询前提、access_context、字段/枚举 | [13](../chapters/13-reflection-model.md)、[F01](../exercises/F01_frontier/README.md) | P1、C05/C12元数据与格式 |
| splicing、展开、define_static_*、define_aggregate、代码生成与存储 | [14](../chapters/14-splicing-generation.md)、[F01](../exercises/F01_frontier/README.md) | 生成字段辅助逻辑；C18语言工具衔接 |
| annotations、attributes区别、C26 pack indexing/fold/constexpr及placement new、C29模板名索引/条件noexcept复合要求、consteval-only DR、P3385提案 | [15](../chapters/15-annotations-frontier.md)、[F01](../exercises/F01_frontier/README.md)、[规范索引](standards-and-implementations.md) | 字段命名/显示策略、C15 UHT区别 |
| 成员指针、字段引用、格式化、拥有型字段列表、输入拒绝/错误通道、反射同域对照 | [16](../chapters/16-static-record.md)、[P1](../exercises/P1_static_record/README.md) | C03错误接口、C05/C12 schema边界 |
| 固定STL invoke/ranges源码入口与退出、下游代表任务遮答案回访 | [17](../chapters/17-source-and-bridges.md)、[源码输入指纹](validation/source-inputs.json) | C06、C09、C10、C14、C15 |
| 值级编译期算法、静态表生成、排序/去重/二分查找、普通consteval参数与NTTP边界、暂存容器不逃逸 | [18](../chapters/18-compiletime-values.md)、[A01](../exercises/A01_compiletime_values/README.md) | C05格式/配置小表、C12 schema查找、C13编译成本观察 |
| 字段选择DSL、结构化字符串NTTP、字段名解析、投影顺序、唯一字段、cvref保持、lvalue-only投影 | [19](../chapters/19-field-projection-dsl.md)、[A02](../exercises/A02_field_projection/README.md) | P1手工metadata、C05展示字段、U01/U02元map案例 |
| 显式对象参数、`std::forward_like`语义、四类cvref、move-only限制、递归lambda和借用生命周期 | [20](../chapters/20-explicit-object-forwarding.md)、[A03](../exercises/A03_explicit_object/README.md) | C06 CPO cvref/noexcept、C10 sender适配器、表达式模板节点转发 |
| 类型组合管线：zip、递归flatten、笛卡尔积、惰性provider、variant返回集合与教学版completion signatures | [21](../chapters/21-type-pipelines.md)、[A04](../exercises/A04_type_pipelines/README.md) | C10 completion signatures主课、错误/停止通道、库接口结果集合计算 |
| 表达式模板：固定向量延迟求值、左值借用/右值拥有、`eval`拥有结果、别名安全赋值、直接写回反例 | [22](../chapters/22-expression-templates.md)、[A05](../exercises/A05_expression_templates/README.md) | C06 ranges/view借用语义、数值库表达式节点、C13代价测量 |
| Boost.Mp11：type list算法、meta map、缺键、插入/替换/更新语义、惰性、运行时索引到类型分派、`mp_map_find`源码 | [23](../chapters/23-mp11.md)、[U01](../exercises/U01_mp11/README.md) | A02字段schema迁移、C10签名集合、C13元查找成本实验 |
| Boost.Hana：编译期值、`type_c`/`integral_c`、string key、异构map、`find`/`at_key`、运行时值借用与MPL历史位置 | [24](../chapters/24-hana.md)、[U02](../exercises/U02_hana/README.md) | C05格式化展示对象、P1记录投影观察、类型/值/对象边界教学 |

## 迁移与边界

本次没有删除旧课程知识。旧课正文及学习文件保留，仅在约定README增加主讲桥接。C03局部泛型说明仍服务其原读者，C04提供系统深讲；C06/C10的具体协议继续由原课负责。字段格式、版本兼容、UHT注册与GC等不因有语言反射就被宣称自动解决。

C++26/C++29各项保留实际语法源码和实验说明；本机不支持时分别列未测/能力SKIP，不能把它们计为运行通过。C++23手工metadata的完整性是注册前提，独立遗漏观察专门说明该边界。

## 审查闭环

复杂样章初审发现“合法ADL fallback正例缺失”和“前置反例未实测”，经r2修复与[独立复验](reviews/sample-review-r2.md)关闭。其余批次的静态发现、作者修复、实际构建及非作者复验逐项保留在reviews和validation；最终报告才汇总当前版本状态，不将静态检查、作者完成和最终批准混成一个PASS。

## 2026-09-10 增量反向映射

本轮增量把“类型计算、值计算、编译期结构驱动运行时代码、预处理裁剪”四类问题拆开讲。A01是值级常量求值和静态结果生成；A02、A04、U01主要是类型计算与结构驱动；A03、A05强调cvref和借用生命周期；C05的fmt/spdlog案例只在下游讲库前端，不把日志系统吞进C04。

硬先修关系如下：A02依赖16章手工字段schema和07/10章NTTP/常量求值；U01依赖08、19章，先有手写schema语义再迁移到Mp11；U02依赖19、23章，观察Hana key/type/value边界；A04依赖06/07/08章的约束、参数包和type_list，并只给C10 completion signatures的教学版集合，不替代C10真实sender语义；A05依赖03/09章的转发、借用表达式和C02生命周期，20章是建议对照，不承诺公开包装为`noexcept`。

下游保留边界：C05接手fmt/spdlog格式化和日志前端；C08/C11后续接手异步日志队列、overflow、flush/shutdown和服务观测；C10接手真实Execution协议；C15接手UHT、GC和UE反射工程约束。C04只给语言与元编程机制，不声明这些下游系统已完成。
