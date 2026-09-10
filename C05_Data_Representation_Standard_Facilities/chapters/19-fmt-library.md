# 19. fmt格式库：格式串检查、代码生成与参数存储边界

第09章先讲标准格式化的输出边界。本章看固定版本 fmt 12.1.0：它是 `std::format` 的重要来源之一，也是很多日志库的格式化后端。学习目标不是“把 `std::` 换成 `fmt::`”，而是沿源码解释三条入口为什么不同。

固定源码：`fmt` commit `407c905e45ad75fc29bf0f9bb7c5c2fd3475976f`。准备脚本把它放在本课 `exercises/build/_deps/fmt-12.1.0/source`，CMake 只接受这个 HEAD 和干净 checkout。

## 固定格式串：在实例化点检查

最小用法：

```cpp
std::string text = fmt::format("{}={}", "id", 7);
```

这个调用看起来像运行时函数，关键约束却在类型里。`include/fmt/base.h` 先把 `fmt::format` 的实参类型推导到 `T...`，再把第一个参数放进 `fstring<T...>`。格式串类型处在这个参数位置，调用者不能单独把它推导成普通 `string_view` 后绕过检查；除非显式选择 `fmt::runtime`。

`fstring` 的构造在常量求值里调用 `parse_format_string` 和 `format_string_checker`。checker 里保存的是格式文本、参数个数、named argument 信息和每个参数映射后的 fmt 内部类型。遇到替换字段时，它用当前位置的参数类型去调用对应 formatter 的 `parse`；`"{:d}"` 配字符串会在这里失败。MSVC 本机诊断表现为 `C7595` immediate-function 失败，调用栈会落到 `report_error`，而不是等程序运行。

`fmt::format_string<T...>` 适合写公共 API：

```cpp
void report(fmt::format_string<std::string_view, int> fmt,
            std::string_view key, int value);
```

这样调用者传 `"{:d}"` 配字符串时，错误留在编译阶段。反例是把固定业务格式串统一改成 `std::string_view` 再 `vformat`；这会把本可早发现的类型错误推到运行期。

手推一次 `fmt::format("{}={}", "id", 7)`：

1. `Args...` 由后两个实参得到字符串和整数相关类型。
2. 第一个实参必须能构造 `fstring<Args...>`。
3. checker 从第一个 `{}` 取第0个参数，确认默认展示可用。
4. 普通字符 `=` 只进入文本段。
5. 第二个 `{}` 取第1个参数，确认整数默认展示可用。
6. 检查完成后，运行期格式化才把值写入输出 buffer。

## 运行时格式串：显式选择运行期错误

配置文件、命令行或用户输入提供的格式串不可能在编译期检查。fmt 用 `fmt::runtime(pattern)` 标出这条路径：

```cpp
auto text = fmt::format(fmt::runtime(pattern), value);
```

错误出口是 `fmt::format_error`。这不是降级，而是信任边界不同：外部文本必须按输入验证处理。`U02_fmt_observation` 会验证 `"{:d}"` 配字符串在运行时被拒绝；编译负例则验证固定格式串同样错误会在构建阶段失败。

## 自定义 formatter 是展示策略

自定义类型格式化只需要特化本类型 formatter：

```cpp
template<>
struct fmt::formatter<Metric> {
    constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const Metric& m, fmt::format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}={}", m.name, m.value);
    }
};
```

`parse` 处理本类型支持的格式说明，`format` 写到输出迭代器。它解决“怎样给人看”，不解决协议编码、字段长度、版本兼容或持久化。C05 的 schema 和包格式仍走显式字段。

## `FMT_COMPILE` 与 `_cf`：解析成编译期表示

`include/fmt/compile.h` 把 `FMT_COMPILE("{}")` 转成 `compiled_string`，再由 `compile_format_string` 生成 `text`、`field`、`spec_field`、`concat` 等类型组合。`_cf` 字面量在支持 class NTTP 时提供同一类入口。

手推 `FMT_COMPILE("{}:{}")`：

1. `compile<T...>(S())` 把字面量转成 `basic_string_view`。
2. 位置0是 `{}`，生成 `field<char, Arg0, 0>`。
3. 后续 `:` 生成 `text<char>` 或单字符 `code_unit<char>`。
4. 最后一个 `{}` 生成 `field<char, Arg1, 1>`。
5. 多段用 `concat<L, R>` 接起来，形成一个编译期格式树。
6. `fmt::format(compiled, args...)` 对这棵树递归调用 `format`，少走运行时格式串解析。

这展示了优雅的编译期值计算：格式文本先被解析成类型和值，再由 `format` 展开。它不等于默认更快。真实收益取决于格式串复杂度、优化器、二进制大小和调用频率；本章只验证正确性和入口差异，不给吞吐结论。

## 参数存储：不要把类型擦除理解成拥有

`make_format_args` 返回 typed store，之后可隐式转成 `format_args` 交给 `vformat`。源码里 `basic_format_args` 是 type-erased view；`format_arg_store` 对小型内置值和对象型参数采用不同存储方式。结论不能简化成“所有参数都是引用”，也不能反过来说它拥有任意业务对象。

需要长期保存动态参数时，优先保存最终字符串。确实要保存参数列表时，`dynamic_format_arg_store` 可以复制普通值；显式放入 `std::ref(x)` 则仍然是借用，`x` 的生命期必须覆盖后续格式化。禁止把 `format_args` 保存到实参生命期之外；编译期格式检查只检查格式文本和类型，不延长对象生命期。

## 练习

[U02 fmt](../exercises/U02_fmt/README.md) 要求运行观察程序、解释源码入口，并完成陌生类型迁移题。检查结果必须包含返回文本、运行期错误通道、编译期负例和参数存储边界。
