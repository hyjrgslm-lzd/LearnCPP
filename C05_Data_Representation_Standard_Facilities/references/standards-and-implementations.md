# C05 规范与实现索引

核对日期2026-09-09。规范、上游实现、本机头文件、编译链接运行是不同证据。当前文件先记录固定输入，执行结果随后链接实际能力记录。

## 规范基线

- C++23 [N4950](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/n4950.pdf)。主线代码请求C++23；介绍byte/charconv/filesystem的C++17起点、bit/chrono/format等C++20设施、byteswap/print等C++23增量。
- C++26 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)及[N5051编辑报告](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5051.html)。最终草案/DIS不是已发布ISO标准的同义词。
- C++29 [N5054](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5054.pdf)与[N5055编辑报告](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html)。C05相关motion包含P3793R2位移、P3104R6位排列、P3395R6 error_code格式与编码、P3505R4浮点格式、P3154R3流的signed character弃用和P3248R5 intptr_t要求；不将simd重讲为C05主线，数值/向量化桥接C13。
- [P2728R14 UTF Transcoding](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p2728r14.html)是本轮跟踪的提案版本。N5055纳入列表未给出其采纳证据；不宣称本机或std已经提供该转码库。

## 本课规范边界

`std::byte`不是Unicode字符或通用算术类型；`bit_cast`转换对象表示，不验证协议，也不使含padding的struct成为可移植数据包。wire明示CHAR_BIT==8、固定字段宽度与大端。

整数转换和表达式溢出分开：整数到整数转换的规则不能用来证明signed加法、移位或浮点越界转换合法。from_chars的成功还需按场景检查ptr是否到末端；它不自动处理协议空白或业务范围。

`std::string`不自行保证UTF8。char8_t区分类型，不自动验证编码；text_encoding识别编码，不执行转码、规范化或分割。标准库的format宽度、码点数和用户可感知字素不是同一度量。

chrono时钟、纪元、单位、范围与时区规则各有契约。tzdb运行可用性、数据库版本、指定zone存在性另记；本课不修改系统时区。filesystem::path native字符类型与编码依赖平台，不能把Windows观察推成所有系统保证，也不能把lexically_normal当沙箱或文件身份验证。

## Unicode 与 ICU

正文编码规则参考[Unicode 16.0核心规范](https://www.unicode.org/versions/Unicode16.0.0/)，规范化[UAX15](https://www.unicode.org/reports/tr15/)，文本分割[UAX29](https://www.unicode.org/reports/tr29/)。滚动附录只作入口，实验固定Unicode16.0数据并保留文件头与版本，避免滚动规则混入旧数据结论。

ICU4C77.1 [官方版本说明](https://unicode-org.github.io/icu/download/77.html)、[固定tag](https://github.com/unicode-org/icu/releases/tag/release-77-1)。本会话git ls-remote回读commit `457157a92aa053e632cc7fcfd0e12f8a943b2d11`。关联Unicode16.0/CLDR47；实际运行须打印u_getVersion/u_getUnicodeVersion。扩展只使用Normalizer2、UnicodeString、root BreakIterator等必要接口，不改变Manifest标识相等规则。

测试材料入口：[NormalizationTest 16.0.0](https://www.unicode.org/Public/16.0.0/ucd/NormalizationTest.txt)、[GraphemeBreakTest 16.0.0](https://www.unicode.org/Public/16.0.0/ucd/auxiliary/GraphemeBreakTest.txt)。ICU和数据许可随各自来源保留，不复制整套第三方源码进入受版本管理的课程目录。

## 能力记录规则

逐设施门槛来自[SD-6](https://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations)及下列固定提案，不用提案中的日期占位符作宏值：

| 设施 | 探测门槛 | 规范依据 |
|---|---|---|
| text_encoding / locale::encoding | __cpp_lib_text_encoding >= 202306L，另检查头 | [P1885R12](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1885r12.pdf) |
| charconv result bool | __cpp_lib_to_chars >= 202306L | [P2497R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2497r0.html) |
| to_string新格式语义 | __cpp_lib_to_string >= 202306L | [P2587R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2587r3.html) |
| runtime_format | __cpp_lib_format >= 202311L | [P2918R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2918r2.html) |
| filesystem::path formatter | __cpp_lib_format_path >= 202403L；202506L增量另分 | [P2845R8](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2845r8.html)、[P2319R5](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2319r5.html) |
| shl/shr | __cpp_lib_bitops >= 202606L | [P3793R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3793r2.html) |
| bit permutations | __cpp_lib_bitops >= 202607L | [P3104R6](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3104r6.html) |

上游STL源码固定为`4edbc1d63a1ec156bed6dbf1727f413fa682abba`，不跟随main滚动。charconv查整数from_chars的无数字/溢出/成功出口；format查basic_format_string、make_format_args及格式输出链；chrono查local_info到time_zone::to_sys；filesystem查u8string/generic_string与平台转换。不存在的前沿入口明确不存在，不用另一同名函数冒充导读。

本轮已只读检查MSVC14.51.36231的yvals_core.h：STL145、update202604，具有byteswap/format/print/charconv等相关宏声明。这是头文件输入，不是C05构建通过。F01须保存真实实例化、链接、运行，缺能力返回77只由capability测试解释为SKIP；一旦宣称能力存在，编译或运行错误是FAIL。ICU未开启记录DISABLED；显式开启缺依赖是配置FAIL，不能SKIP。

上游源码导读采用microsoft/STL的固定提交，读取charconv、format、chrono、filesystem与yvals_core.h；本机头文件SHA不等同上游Git提交。精确导读锚点及运行对照在第17章交付，现阶段不声称已完成源码阅读。
