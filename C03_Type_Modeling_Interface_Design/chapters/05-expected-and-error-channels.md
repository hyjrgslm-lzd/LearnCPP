# 05：`std::expected` 与错误通道

错误通道回答两个问题：失败怎样传回调用者，调用者能拿到什么信息。异常和 `std::expected<T, E>` 都能表达失败，但它们给接口读者的承诺不同。异常适合当前层通常不能恢复、或错误路径会淹没主流程的情况；`expected` 适合调用者经常要按错误分类分支处理的业务失败。

```cpp
std::expected<UserId, ParseUserIdError> parse_user_id(std::string_view text);
```

这个签名说明：成功得到 `UserId`，失败得到 `ParseUserIdError`。它比 `optional<UserId>` 多了失败原因，比 `bool parse(text, out)` 更难丢失错误信息。它仍没有自动说明对象失败后处于什么状态；如果函数会修改对象，必须另写异常安全或事务保证。第 06 章会把“错误能传出”和“对象状态没坏”分开验证。

## `expected<T, E>` 与 `unexpected<E>`

`std::expected<T, E>` 在任一时刻持有一个 `T` 值，或一个 `E` 错误。成功状态可从 `T` 构造；错误状态用 `std::unexpected<E>` 构造，避免把错误值误当成功值。

```cpp
enum class ParseError { empty, invalid_digit, out_of_range };

std::expected<int, ParseError> parse_count(std::string_view text) {
    if (text.empty()) {
        return std::unexpected(ParseError::empty);
    }
    // ...
    return 42;
}
```

`T` 和 `E` 都是类型建模的一部分。`E` 不应只是 `std::string` 的随手消息，除非调用者只会展示文本。若调用者要重试、降级、统计或映射到协议状态码，错误类型应该可分类，例如 `enum class` 加必要上下文结构。

`expected<void, E>` 表达“成功没有值，但失败有错误”。它适合命令式操作：

```cpp
std::expected<void, EditError> apply(Edit edit);
```

这比 `bool apply(Edit)` 更完整，也比“失败抛异常”更适合常见业务拒绝，例如重复 ID、缺失 ID、非法尺寸。成功通道是 `void`，但状态仍有两种：成功或错误。

## observer：前提与定义好的抛异常路径

`expected` 的布尔上下文检查是否成功。成功时可用 `*e`、`e->member` 或 `e.value()` 取值；失败时用 `e.error()` 取错误。但这些 observer 不是同一类：有些有前提，违反就是程序错误；有些会在错误状态走定义好的抛异常路径。

```cpp
auto parsed = parse_count(text);
if (!parsed) {
    return std::unexpected(parsed.error());
}
use(*parsed);
```

分组要分清：

- `*e` 和 `e->` 是 unchecked observer，前提是 `e.has_value()` 为真；错误状态下使用违反前提。
- `e.error()` 的前提是 `!e.has_value()`；成功状态下使用违反前提。
- `e.value()` 是 checked observer；错误状态下按定义抛 `std::bad_expected_access<E>`，异常对象携带错误值。
- `value_or(default_value)` 在失败时返回默认值；它没有访问前提问题，但默认实参会先求值，且默认值要能转换成 `T`。
- `error_or(default_error)` 在成功时返回默认错误；它没有访问前提问题，但默认实参同样会先求值，且默认错误要能转换成 `E`。

`expected<T, E>` 的“never-empty”只表示对象总能表示成功或错误之一，不表示访问没有前提，也不表示操作不会抛。构造、移动、拷贝、转换、回调都可能按 `T` 或 `E` 的行为抛异常。

## 错误码、异常和分层映射

一个常用分层规则是：本层能预期并处理的业务失败用 `expected`；违反底层资源假设或不打算在本层恢复的问题用异常传播。解析用户输入时，空文本和非法字符是业务失败；分配失败通常不是解析器本层能恢复的业务错误。

```cpp
std::expected<UserId, ParseError> make_user_id(std::string_view text);
std::expected<void, EditError> Document::apply(Edit edit);
```

工厂函数比“默认构造后 set”更容易保持不变量。若 `UserId` 必须非零，`make_user_id` 可以返回 `expected<UserId, ParseError>`；构造成功的 `UserId` 永远有效，失败原因明确传出。第 01 章的不变量和本章错误通道在这里合并。

分层系统需要错误映射。低层 `ParseError::invalid_digit` 可以映射成应用层 `EditError::invalid_id`；低层 `std::errc::permission_denied` 可以映射成协议层 `Status::forbidden`。映射要保留调用者需要的信息，不能把所有错误压成 `failed`。

异常也能进入错误通道，但要显式处理。比如异步框架常用 `std::exception_ptr` 存储跨线程异常，后续在完成通道中传回。C09 会讲协程 promise 如何存储异常，C10 会讲 sender/receiver 的 value/error/stopped 完成通道。本章只要求你知道：`expected<T, std::exception_ptr>` 可以保存“稍后重抛”的异常对象，但这不是业务错误建模的默认选择。

## C++23 monadic 操作

C++23 `expected` 提供 `and_then`、`transform`、`or_else`、`transform_error`。它们与 `optional` 的链式操作相似，但保留错误负载。

`and_then(f)` 只在成功时调用 `f(value)`，且 `f` 返回另一个 `expected<U, E>`。失败时原错误短路传递。

```cpp
auto result = parse_user_id(text)
    .and_then([&](UserId id) { return document.find_editable(id); })
    .and_then([&](Element element) { return validate(element); });
```

`transform(f)` 只在成功时调用 `f(value)`，把返回值包装成 `expected<U, E>`；失败时错误不变。它适合成功值换形状，但错误语义不变。

`or_else(f)` 只在失败时调用 `f(error)`，且返回兼容的 `expected<T, E2>`。它适合按错误恢复或替换错误通道。

`transform_error(f)` 只在失败时调用 `f(error)`，把错误映射成新错误类型；成功值不变。

回调抛异常时，异常直接传播，`expected` 不会自动捕获成 `unexpected`。若要把异常转成错误值，必须在回调或外层显式 `try/catch`。这条边界很重要：monadic 链减少分支样板，不是异常安全框架。

## move-only、const 和借用实参

`expected` 的可复制、可移动能力由 `T` 与 `E` 决定。`expected<std::unique_ptr<T>, Error>` 可移动不可复制；链式操作要用 `std::move(e).and_then(...)` 才能把成功值移入下一步。对 `const expected<T,E>&` 调用时，回调看到的是 `const T&` 或 `const E&`；不能从中移动资源。

C++23 `std::expected<T, E>` 禁止 `T` 是引用类型，`std::expected<int&, Error>` 是 ill-formed。需要借用时，用指针、`std::reference_wrapper<T>` 或显式 observer 类型表达“可能没有值但不拥有对象”。

C++23 主线若要表达“成功时借用一个已有对象，失败时返回错误”，可用 `std::expected<std::reference_wrapper<T>, E>`、指针、迭代器或回调访问。`reference_wrapper<T>` 只是一个可复制的引用包装，不延长被引用对象生命期；容器重分配、删除元素或被借用对象析构后，包装仍会悬垂。

`const std::reference_wrapper<T>` 也不等于 `std::reference_wrapper<const T>`。前者是包装对象本身不可重新赋值，但 `get()` 仍返回 `T&`，可以修改 pointee；后者包装的是 `const T`，调用者只能读。接口若要承诺只读借用，应写 `std::reference_wrapper<const T>`。若要返回值快照，使用 `expected<T, E>`。

## 与 C03 Document 的关系

C03 最终 `Document` 的编辑接口会使用：

```cpp
std::expected<void, EditError> apply(Edit edit);
std::expected<void, EditError> apply_batch(std::span<const Edit> edits);
```

重复 Add、缺失 Replace、缺失 Erase 都是调用者可预期处理的业务拒绝，所以进入 `EditError`。复制或分配异常不是 `EditError`，允许传播；但第 06 章会要求文档在异常传播后仍保持调用前状态。这里能看到两个轴：错误通道负责解释失败原因，异常安全负责解释对象状态。

## L05 练习

`L05_expected` 是观察型练习，覆盖：

- `expected<T,E>` 与 `expected<void,E>` 的成功/错误状态。
- `unexpected` 构造错误，避免错误值被误作成功值。
- `*`、`->`、`error()` 的前提，`value()` 的定义好抛异常路径，以及 `value_or()`、`error_or()` 的默认实参求值。
- C++23 借用返回使用 `expected<reference_wrapper<T>, E>`，不是 `expected<T&, E>`。
- 错误分类和跨层 `transform_error` 映射。
- `and_then/transform/or_else/transform_error` 的短路、返回要求和异常传播。
- move-only 成功值和 `const` 访问限制。

完整解析：`expected` 适合“调用者应该看见并处理”的失败。它不是比异常更高级的替代品，也不自动提供事务保证。正确接口要同时说明错误通道、对象状态、资源所有权和异常传播边界。
