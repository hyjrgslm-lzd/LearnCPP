# U02 fmt 源码观察与迁移

对应正文：[第19章 fmt格式库](../../chapters/19-fmt-library.md)。

本单元只做观察和迁移，不提供 Student/Reference 占位。运行 `U02_fmt_observation`，确认：

- 固定 `fmt::format_string` 在实例化点检查格式串和参数类型。
- `fmt::runtime` 把格式文本改为运行期检查，错误进入 `fmt::format_error`。
- 自定义 `fmt::formatter<T>` 是展示策略，不替代 wire 编码。
- `FMT_COMPILE` / `_cf` 可把字面量解析成编译期格式表示，但不是默认速度承诺。
- `make_format_args` 是类型擦除入口，内置值和非拥有对象的存储边界不能混成“全部引用”。
- `dynamic_format_arg_store` 可以保存复制进去的动态参数；显式传 `std::ref` 时仍是借用。

迁移题：把 `SensorReading { std::string name; int milli; }` 格式化为 `name=1.234`。要求固定格式串走编译期检查；来自配置文件的格式串必须显式走 `fmt::runtime` 并处理 `format_error`；不要缓存悬垂的 `format_args`。若使用 `dynamic_format_arg_store`，说明哪些参数被复制，哪些通过 `std::ref` 借用。

负例 `diagnostics/bad_compile_format.cpp` 使用固定格式串把字符串按十进制整数格式化，应在编译阶段被拒绝。配套 control 先证明同一依赖和工具链可正常编译。
