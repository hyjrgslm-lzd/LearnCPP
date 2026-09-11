# C05 覆盖与下游反向追踪

以获准全局计划C05行及重构指南验收，不以文件数计完成。全部为新增内容；旧课知识不删除，只增加C03/C09/C15导航桥接。规范/宏/运行状态见[规范索引](standards-and-implementations.md)，复验命令和检查范围见构建指南。

| 知识问题 | 主讲与练习 | 下游必须获得的能力与检查 |
|---|---|---|
| 路线与责任边界 | [00章](../chapters/00-roadmap.md) | 先修与任务责任清楚，无C04循环先修 |
| 字节与对象表示 | [01章](../chapters/01-bytes-and-object-representation.md) / [L01_bytes](../exercises/L01_bytes/README.md) | 区分对象表示、padding/别名/存活与可移植wire；安全观察不冒充UB复现 |
| 位操作、端序与有界读取 | [02章](../chapters/02-bits-and-endian.md) / [L02_bits_endian](../exercises/L02_bits_endian/README.md) | 显式端序、位宽、bool排除、size-pos前置与游标边界 |
| 整数转换与数值边界 | [03章](../chapters/03-integer-conversions.md) / [L03_integer_boundaries](../exercises/L03_integer_boundaries/README.md) | 提升/有符号混合/转换不同于signed溢出，数值边界拒绝 |
| 字符串、借用与char8_t | [04章](../chapters/04-strings-and-views.md) / [L04_strings_views](../exercises/L04_strings_views/README.md) | string_view借用、NUL与长度、char8_t类型和编码验证分层 |
| Unicode单位与编码模型 | [05章](../chapters/05-unicode-model.md) / [L05_unicode](../exercises/L05_unicode/README.md) | 码元/标量/字素、UTF8长度/overlong/代理项/BOM/noncharacter |
| 严格UTF转码、错误坐标和预算 | [06章](../chapters/06-utf-transcoding.md) / [L06_transcoding](../exercises/L06_transcoding/README.md) | 严格双向转码、输入输出预算、相应输入单位坐标；非法输入和极值 |
| 规范化、大小写、字素 | [07章](../chapters/07-unicode-text-semantics.md) / [L07_text_semantics](../exercises/L07_text_semantics/README.md) | 规范化四形式、casefold/locale/字素与字节身份不同，U01官方数据真实消费 |
| 完整消费与数值解析 | [08章](../chapters/08-parsing-and-charconv.md) / [L08_parsing](../exercises/L08_parsing/README.md) | from_chars成功还检查ptr，符号/空白/溢出/finite和错误位置 |
| 格式化、动态参数、locale | [09章](../chapters/09-formatting-and-locales.md) / [L09_formatting](../exercises/L09_formatting/README.md) | 固定/动态format、参数寿命、缓冲截断、locale及非ASCII宽度边界 |
| 时钟、单位、精度和范围 | [10章](../chapters/10-clocks-and-durations.md) / [L10_clocks_durations](../exercises/L10_clocks_durations/README.md) | system/steady、不同epoch、duration精度/floor/范围 |
| 日历、UTC、本地时间与时区 | [11章](../chapters/11-calendars-and-time-zones.md) / [L11_calendar_zones](../exercises/L11_calendar_zones/README.md) | 日期有效性、UTC不依赖tzdb、歧义与不存在时间、历史秒偏移、显示范围 |
| 路径表示、原生编码与文件身份 | [12章](../chapters/12-filesystem-paths.md) / [L12_paths](../exercises/L12_paths/README.md) | generic元数据vsnative文件路径，词法与文件身份区别；不当作完整sandbox |
| 严格有界配置与诊断 | [13章](../chapters/13-configuration.md) / [L13_configuration](../exercises/L13_configuration/README.md) | 有界key=value，CRLF/裸CR/非法UTF/NUL行号、重复未知键与设备basename拒绝 |
| 有界UTF8字段与提交点 | [14章](../chapters/14-bounded-binary-fields.md) / [L14_binary_fields](../exercises/L14_binary_fields/README.md) | 正确字节基线到UTF字段、失败不提交、每截断点、全buffer坐标 |
| schema、旧新版本与信息损失 | [15章](../chapters/15-schema-evolution.md) / [L15_schema_evolution](../exercises/L15_schema_evolution/README.md) | schema/wire/对象布局分层；v1/v2/future minor、unknown重复与丢失、独立golden |
| 资源清单包综合项目 | [16章](../chapters/16-resource-manifest.md) / [P1_resource_manifest](../exercises/P1_resource_manifest/README.md) | 配置到真实有界文件回读，拥有型结果、空清单zone、输出不覆盖、cleanup优先 |
| 固定源码与下游反向检查 | [17章](../chapters/17-source-and-downstream.md) | 固定charconv/format/chrono/filesystem入口/状态/失败出口，反查C03/C09/C11/C12/C15 |
| C++26/C++29与提案边界 | [18章](../chapters/18-standard-frontier.md) / [F01_frontier](../exercises/F01_frontier/README.md) | 逐设施DISABLED/SKIP/PASS/FAIL，已入草案和P2728提案不混淆 |
| fmt格式库前端、`format_string`编译期校验、`fmt::runtime`运行期错误、`FMT_COMPILE`/`_cf`、自定义formatter和参数存储边界 | [19章](../chapters/19-fmt-library.md) / [U02_fmt](../exercises/U02_fmt/README.md) | C04第10/18章的常量求值背景可辅助理解，但本课主讲格式输出、错误通道和参数生命周期；C09日志/RPC输出可复用运行期格式拒绝规则 |
| spdlog同步前端、局部logger/sink、`SPDLOG_ACTIVE_LEVEL`预处理裁剪、运行时level过滤、后端formatter、pattern runtime配置和error_handler | [20章](../chapters/20-spdlog-frontend.md) / [U03_spdlog](../exercises/U03_spdlog/README.md) | 只覆盖同步日志前端；异步queue、overflow、flush/shutdown与服务观测转交C08/C11 |

## 下游回访与独立性

P1实际使用的bytes/UTF、整数表示、Config错误和路径、chrono、versioned codec均能沿00—16的硬先修获得；P1允许复用已交付低层操作，作业对象是实际组合与失败传播，不要求重做每个算法。L15独立good不调用canonical codec，旧读者只理解tags1—5；未知字段的跳过不等于保存其信息。

C03内存Document工厂之外的输入转换由本课主讲。C09 RPC的数字完整消费、长度头与资源预算有对应正文和反例；连接/取消/调度仍归C09/C11。C12接手持久化和深格式兼容；C15 FString/FName/FText/FArchive需要其实际UE版本规则，通用Unicode基础不证明所有平台的TCHAR布局。

fmt/spdlog增量是C05自己的库前端案例：第19章承接09章`std::format`，讲fmt12.1.0的编译期格式串、运行期格式串和参数存储；第20章承接第19章，讲spdlog1.17.0如何把格式后端接到同步logger/sink。它们不反向要求读者先完成C04 Mp11/Hana；C04只提供“为什么格式串可以变成编译期检查/生成结构”的语言背景。

## 自查与边界

复杂样章、文本/时间/ICU/前沿、schema/P1和全课教学分别需要正文解释、练习入口、good/bad控制体和能力边界共同支撑。不要把静态检查、作者完成和运行结果混成一个通过计数。

其他平台、本机缺实现的前沿和未覆盖的真实I/O故障方式保持明确未验证，不通过减少章节规避义务。
