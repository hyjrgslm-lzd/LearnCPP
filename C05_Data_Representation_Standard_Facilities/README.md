# C05：数据表达与常用标准设施

2026-09-10 的 fmt/spdlog 跨课增量已完成，现有21章/20单元。当前结果见[增量质量报告](references/revision-quality-report-20260910.md)与[联合交付清单](../C04_Generic_CompileTime_Reflection/references/revision-delivery-manifest.md)；前次19章基线保留历史验证记录。

以资源清单与数据包贯穿字节、编码、解析、时间、路径和schema。面向已有C++经验的工程师，正文连续推导机制，练习验证理解，标准规则与本机观察分别说明。

实际验证、非作者审查及未测边界见[质量报告](references/quality-report.md)；逐知识点闭环见[覆盖表](references/coverage.md)，接口与任务边界见[实施规格](references/implementation-spec.md)。目录或测试数量不代替教学与技术验收。

## 怎样开始

先具备[C01最小构建](../C01_Build_Compile_Link/README.md)、[C02对象与借用](../C02_Objects_Lifetime_Ownership/README.md)、[C03错误通道](../C03_Type_Modeling_Interface_Design/README.md)。当前需要的局部模板规则在本课解释，C04不是隐藏硬先修。按00—06建立基础，再进入文本、时间、路径和字段链；14章是已独立审查的复杂样章，16章将各层接起来。

| 章 | 正文 | 对应练习 |
|---|---|---|
| 00 | [路线与责任边界](chapters/00-roadmap.md) | 自测与跨章回访 |
| 01 | [字节与对象表示](chapters/01-bytes-and-object-representation.md) | [L01_bytes](exercises/L01_bytes/README.md) |
| 02 | [位操作、端序与有界读取](chapters/02-bits-and-endian.md) | [L02_bits_endian](exercises/L02_bits_endian/README.md) |
| 03 | [整数转换与数值边界](chapters/03-integer-conversions.md) | [L03_integer_boundaries](exercises/L03_integer_boundaries/README.md) |
| 04 | [字符串、借用与char8_t](chapters/04-strings-and-views.md) | [L04_strings_views](exercises/L04_strings_views/README.md) |
| 05 | [Unicode单位与编码模型](chapters/05-unicode-model.md) | [L05_unicode](exercises/L05_unicode/README.md) |
| 06 | [严格UTF转码、错误坐标和预算](chapters/06-utf-transcoding.md) | [L06_transcoding](exercises/L06_transcoding/README.md) |
| 07 | [规范化、大小写、字素](chapters/07-unicode-text-semantics.md) | [L07_text_semantics](exercises/L07_text_semantics/README.md) |
| 08 | [完整消费与数值解析](chapters/08-parsing-and-charconv.md) | [L08_parsing](exercises/L08_parsing/README.md) |
| 09 | [格式化、动态参数、locale](chapters/09-formatting-and-locales.md) | [L09_formatting](exercises/L09_formatting/README.md) |
| 10 | [时钟、单位、精度和范围](chapters/10-clocks-and-durations.md) | [L10_clocks_durations](exercises/L10_clocks_durations/README.md) |
| 11 | [日历、UTC、本地时间与时区](chapters/11-calendars-and-time-zones.md) | [L11_calendar_zones](exercises/L11_calendar_zones/README.md) |
| 12 | [路径表示、原生编码与文件身份](chapters/12-filesystem-paths.md) | [L12_paths](exercises/L12_paths/README.md) |
| 13 | [严格有界配置与诊断](chapters/13-configuration.md) | [L13_configuration](exercises/L13_configuration/README.md) |
| 14 | [有界UTF8字段与提交点](chapters/14-bounded-binary-fields.md) | [L14_binary_fields](exercises/L14_binary_fields/README.md) |
| 15 | [schema、旧新版本与信息损失](chapters/15-schema-evolution.md) | [L15_schema_evolution](exercises/L15_schema_evolution/README.md) |
| 16 | [资源清单包综合项目](chapters/16-resource-manifest.md) | [P1_resource_manifest](exercises/P1_resource_manifest/README.md) |
| 17 | [固定源码与下游反向检查](chapters/17-source-and-downstream.md) | 自测与跨章回访 |
| 18 | [C++26/C++29与提案边界](chapters/18-standard-frontier.md) | [F01_frontier](exercises/F01_frontier/README.md) |
| 19 | [fmt格式检查、生成与参数存储](chapters/19-fmt-library.md) | [U02_fmt](exercises/U02_fmt/README.md) |
| 20 | [spdlog同步前端与宏裁剪](chapters/20-spdlog-frontend.md) | [U03_spdlog](exercises/U03_spdlog/README.md) |

[U01 ICU扩展](exercises/U01_icu_unicode/README.md)配合07章，独立完成Unicode16.0规范化与字素测试。核心构建不依赖ICU；扩展默认关闭，显式开启缺依赖就是失败。9项前沿能力在本机有明确未支持记录，不把SKIP当作设施运行通过。

19/20是格式库进阶路线，先掌握09章与C04相应的推导、常量求值和类型组合知识；不要求基础学习者提前学完C04。fmt/spdlog通过`format-libs`和`format-libs-std`独立预设验证，默认核心不依赖它们。两个单元是观察/源码阅读/迁移题，没有Student占位；异步日志运行时由C08/C11后续主讲。

## 练习和构建

7个实现型单元为L03、L06、L08、L13、L14、L15、P1；其余基础专题为观察/解释型，U01为ICU扩展验证，F01为逐设施能力入口。学生只改各题src/student；Reference、独立good完成体、bad控制体及观察程序的通过含义不同。

从exercises目录按[构建指南](exercises/BUILD_GUIDE.md)运行Release、Debug、Student、ASan、frontier或ICU预设。整课与单题均在完整checkout中构建；复用C01/C02检查工具，不安装机器级组件。

## 主案例的边界

Manifest包含资源ID、UTF8名称与相对路径、字节数、UTC Unix毫秒以及可选备注。包格式明确大端与宽度，保留独立旧读者及[手写黄金输入](exercises/fixtures/README.md)。资源path只作元数据，P1只在自有临时目录读写新包，不打开资源目标、不覆盖既有文件。

ICU规范化与大小写处理不改变Manifest的字节身份。C09 RPC可承接长度/解析基础，C11/C12继续网络和存储机制，C15保留UE专有对象、编码和资产责任。本课不把这些下游课程纳入完成声明。
