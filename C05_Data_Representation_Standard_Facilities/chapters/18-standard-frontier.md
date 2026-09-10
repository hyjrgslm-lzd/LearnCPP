# 18 标准前沿：识别、输出和位操作的新边界

C05 的正文和练习以 C++23 可用设施为主。本章处理另一类知识：已经进入 C++26 或 C++29 草案、但本机标准库未必实现的设施。学习目标不是追新语法，而是学会把“标准已采纳”“feature-test macro 已声明”“头文件可包含”“代码能实例化、链接、运行”分开记录。

本章对应练习是 `exercises/F01_frontier`。它默认关闭；`DATA_STUDY_ENABLE_FRONTIER=OFF` 时 configure 只报告 `F01 disabled; no capability tests registered`，不注册 F01 测试，状态记为 DISABLED。显式打开后，每个设施单独生成一个 executable。头文件或 macro 缺失返回 77；macro 宣称支持后才编译真实调用并运行最小语义检查。这样可以避免两种误读：把“没有实现”写成失败，也避免把“宏存在”写成已经支持。

## 怎么读 F01 输出

`DISABLED` 表示没有启用前沿探测，不能推出能力结论。`PASS` 表示本机工具链完成了编译、链接和运行，并且最小语义检查符合本章记录的标准意图。`SKIP` 表示显式探测时这个工具链还没有声明或没有通过能力探测；它不是通过。`FAIL` 表示工具链声称到达对应阈值，或通过了配置期探测，但实际编译/运行语义不符合预期。

当前本机基线是 MSVC 19.51.36256.0、STL include 目录 `D:/VisualStudio2026/Installed/VC/Tools/MSVC/14.51.36231/include`。固定上游源码锚点仍按 MSVC STL commit `4edbc1d63a1ec156bed6dbf1727f413fa682abba` 阅读；本机版本和这个 commit 要分开写，不能互相代替。

## `std::text_encoding` 和 `std::locale::encoding`

C++26 的 `std::text_encoding` 解决的是“我现在拿到的名字表示哪种字符编码”这个识别问题。它不是转码库，也不会把 UTF-8 转成 UTF-16。P1885R12 给出 `<text_encoding>`、`std::text_encoding::literal()`、`environment()`、`mib()`、`name()`、`aliases()` 这类接口；`std::locale("").encoding()` 则把 locale 关联的编码也变成同一类可比较对象。SD-6 对应 `__cpp_lib_text_encoding == 202306L`。

最小调用路径：

```cpp
#include <locale>
#include <text_encoding>

auto literal = std::text_encoding::literal();
auto env = std::text_encoding::environment();
auto loc = std::locale("").encoding();
bool literal_is_utf8 = literal.mib() == std::text_encoding::UTF8;
```

反例是把它当转码器使用，例如期待 `text_encoding("utf8")` 提供“转 UTF-16”的成员函数。C05 的转码仍由第06章严格 UTF-8/UTF-16 代码和 U01 ICU 扩展承担；P2728R14 本轮未按已查来源确认进入 N5054/C++26，所以不能写成标准 `std` 转码实现。

F01 的 `F01_text_encoding_locale` 先检查 `<text_encoding>` 和 `__cpp_lib_text_encoding`。缺失时 SKIP；存在时构造 UTF-8、literal、environment、locale encoding，并检查名字非空与 UTF-8 识别。

## `charconv` 结果可直接判成功

C++17 的 `std::from_chars` / `std::to_chars` 返回 `{ptr, ec}`。老写法必须检查 `ec == std::errc{}`，再看 `ptr` 是否消费到期望位置。P2497R0 给结果类型加入显式 bool 转换，SD-6 把 `__cpp_lib_to_chars` bump 到 `202306L`。

最小调用：

```cpp
int value = 0;
auto r = std::from_chars(first, last, value);
if (!r) {
    // invalid_argument 或 result_out_of_range
}
```

反例是只写 `if (r)` 后忘记检查 `r.ptr == last`。bool 只表示转换本身成功，不表示完整消费。C05 第08章仍要求外层解析器检查 full consumption。

F01 的 `F01_charconv_result_bool` 在 macro 到达 `202306L` 后才编译 `static_cast<bool>(result)`，并分别检查成功和空输入失败路径。本机 `__cpp_lib_to_chars` 仍是 `201611L`，所以该项 SKIP。

## `std::to_string` 新语义

旧 `std::to_string(1.0)` 常见输出是 `"1.000000"`，这来自 C 风格格式化习惯，不适合做稳定展示文本。P2587R3 把 `to_string` 改到接近 `std::format("{}", value)` 的语义，SD-6 对应 `__cpp_lib_to_string == 202306L`。

最小调用：

```cpp
std::string s = std::to_string(1.0); // C++26 后预期更接近 "1"
```

反例是把 `to_string` 输出当 wire 格式。无论新旧语义，它都不是协议编码；C05 的二进制包和配置解析仍要显式字段、长度、端序和错误位置。

F01 的 `F01_to_string_semantics` 要求 `__cpp_lib_to_string >= 202306L`，并把 `std::to_string(1.0)`、`std::to_string(1.25)` 与 `std::format` 比较。本机没有该宏，SKIP。

## `std::runtime_format`

C++20/23 的 `std::format` 默认要求格式串在编译期检查。动态格式串要走 `std::vformat(pattern, args)`，而 `make_format_args` 保存的是参数引用；把临时对象塞进长期保存的 `format_args` 是典型生命周期错误。P2918R2 增加 `std::runtime_format`，让动态格式串走 `std::format` 调用形状，同时保留类型检查边界。SD-6 对应 `__cpp_lib_format >= 202311L`。

最小调用：

```cpp
std::string pattern = "{} + {} = {}";
std::string text = std::format(std::runtime_format(pattern), 2, 3, 5);
```

C++23 保留路径是：

```cpp
int a = 2, b = 3, c = 5;
auto args = std::make_format_args(a, b, c);
std::string text = std::vformat("{} + {} = {}", args);
```

反例是返回 `std::make_format_args(std::string{"x"})` 或把 `format_args` 存到参数生命期之外。`runtime_format` 解决动态格式串入口，不解决参数对象的生命期管理。

F01 的 `F01_runtime_format` 在 `__cpp_lib_format >= 202311L` 后编译真实 `std::runtime_format` 调用，同时保留一个 C++23 `vformat` 对照。本机 `__cpp_lib_format` 是 `202304L`，SKIP。

## `std::filesystem::path` formatter 和 202506 表示修正

路径有两个维度：路径元素的原生表示，以及给人看的显示文本。C++23 里 `path::string()` / `generic_string()` 在 Windows 上容易穿过当前代码页，导致 mojibake 或丢失。P2845R8 给 `std::filesystem::path` 增加 formatter，SD-6 对应 `__cpp_lib_format_path == 202403L`；默认 `{}` 用于显示，`{:?}` 用转义展示控制字符，`{:g}` 走 generic 格式。

P2319R5 进一步把 `path::string()` / `generic_string()` 的问题显式化，加入 `display_string()` / `system_encoded_string()` 以及 generic 对应版本，并要求 bump `__cpp_lib_format_path`。SD-6 记录该项后续值为 `202506L`。

最小调用：

```cpp
std::filesystem::path p{"alpha/beta.txt"};
auto display = std::format("{}", p);
auto generic = std::format("{:g}", p);
auto escaped = std::format("{:?}", std::filesystem::path{"multi\nline"});
```

反例是为了日志直接写 `p.string()`，然后假设它是 UTF-8。C05 第12章对资源路径仍要求把“词法路径字符串”和“文件系统对象身份”分开；本课程 Manifest 的路径字段只是 UTF-8 generic 相对路径元数据，不拿它直接打开资源。

F01 的 `F01_path_formatter` 在 `__cpp_lib_format_path >= 202403L` 后编译 `{}`、`{:g}`、`{:?}`；到 `202506L` 后再编译 `display_string()` 和 `system_encoded_string()`。本机没有 `__cpp_lib_format_path`，SKIP。

## C++29 `std::format` / `std::to_chars` 修正

N5055 的 LWG Poll 6 接受 P3395R6：修正编码问题并给 `std::error_code` 增加 formatter。LWG Poll 7 接受 P3505R4：修正默认浮点表示，同时影响 `std::format` 和无格式参数的 floating `std::to_chars`。这两项属于 C++29 working draft；本轮没有可靠、单独的 SD-6 macro 能直接覆盖“error_code formatter 可用”和“浮点默认表示已改”两个语义，所以 F01 用真实探测。

P3505 的旧问题可以用两个输出看出来：

```cpp
std::format("{}", 100000.0);
std::format("{}", 1234567890123456700000.0);
```

旧实现可能输出 `"1e+05"` 或长串“垃圾数字”；P3505 期望按指数范围选择更可读、仍可 round-trip 的表示。F01 的 `F01_format_float_c29` 直接运行这两个格式化，符合新语义 PASS；观察到旧语义则 SKIP，并打印实际输出。本机输出为 `1e+05` 和 `1234567890123456774144`，所以 SKIP。

P3395 的最小调用是：

```cpp
std::error_code ec = std::make_error_code(std::errc::invalid_argument);
std::string text = std::format("{}", ec);
```

本机标准库里 `std::formattable<std::error_code, char>` 不能作为可靠判断；实际 `std::format("{}", ec)` 会触发 formatter 成员缺失错误。F01 因此在 CMake 配置期用 `check_cxx_source_compiles` 编译真实调用，通过才给目标定义 `C05_HAS_ERROR_CODE_FORMATTER`。本机配置期探测失败，运行项 SKIP。

## C++29 `std::shl` / `std::shr`

内建 `<<` / `>>` 遇到负位移或位移量大于等于宽度时有未定义行为。P3793R2 给 `<bit>` 增加 `std::shl` 和 `std::shr`，目标是宽契约：过长位移给数学上自然的 0 或符号扩展结果，负位移按相反方向处理。N5055 记录 `__cpp_lib_bitops` 被 bump 到 `202606L`。

最小调用：

```cpp
auto a = std::shl(std::uint32_t{1}, 32); // 0
auto b = std::shr(std::uint32_t{8}, -1); // 16
```

反例是把它当性能优化替代所有移位。它解决 UB 边界和可读性；热路径是否改用它，仍须按[全局性能实验与量化规则](../../CONTENT_REFACTORING_GUIDE.md#11-性能实验与量化结果)取得证据，进一步的性能主讲归C13。

F01 的 `F01_bitops_shift_c29` 在 `__cpp_lib_bitops >= 202606L` 后检查过长和负位移。本机 `__cpp_lib_bitops` 是 `201907L`，SKIP。

## C++29 bit permutation

P3104R6 给 `<bit>` 增加 `std::bit_reverse`、`std::bit_repeat`、`std::bit_compress`、`std::bit_expand`。它们把常见但容易写错的位排列表达成标准库调用。N5055 记录 `__cpp_lib_bitops` 再 bump 到 `202607L`。P3772R2 给 SIMD 版本增加对应能力，N5055 记录 `__cpp_lib_simd_bitops == 202607L`；C05 不在 F01 引入 SIMD 依赖，SIMD 留给 C13/C08 相关课程。

最小调用：

```cpp
auto r = std::bit_reverse(std::uint32_t{0x00001234}); // 0x24c80000
auto m = std::bit_repeat(std::uint32_t{0xc}, 4);      // 0xcccccccc
auto c = std::bit_compress(std::uint32_t{0b1101}, std::uint32_t{0b0101});
auto e = std::bit_expand(std::uint32_t{0b11}, std::uint32_t{0b0101});
```

反例是为了今天一个固定掩码手写一套泛化位排列库。C05 只需要知道标准库正在补这个缺口；没有实现时，不要在课程里造一个“std 兼容层”冒充未来标准。

F01 的 `F01_bitops_permutation_c29` 要求 `__cpp_lib_bitops >= 202607L` 后才编译真实调用。本机 SKIP。

## 源码阅读路径

固定 MSVC STL commit `4edbc1d63a1ec156bed6dbf1727f413fa682abba` 可用于第17章源码导读衔接：

- `stl/inc/charconv`：整数 `from_chars` 入口在 `_Integer_from_chars`，失败出口包括无数字、溢出和成功提交；浮点入口是 `_Floating_from_chars`。
- `stl/inc/format`：`basic_format_string` 做编译期解析，`make_format_args` 暴露引用生命期边界，`vformat_to` / `vformat` 是运行期格式串路径。
- `stl/inc/filesystem`：`u8string()`、`generic_string()` 和 `_Path_iterator` 展示路径表示转换；该 commit 没有 path formatter。
- `stl/inc/chrono`：`time_zone::to_sys` 展示 ambiguous/nonexistent local time 的出口；它和本章 encoding/format 状态是不同轴，不要混在一个“标准库支持”结论里。

本章官方入口：SD-6 feature-test macro 表；P1885R12、P2497R0、P2587R3、P2918R2、P2845R8、P2319R5；N5055；P3395R6、P3505R4、P3793R2、P3104R6、P3772R2。课程正文只摘取接口和状态，不复制规范大段文字。
