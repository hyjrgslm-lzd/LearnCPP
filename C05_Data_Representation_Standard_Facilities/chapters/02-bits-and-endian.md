# 02：位运算、端序与显式整数格式

对象表示告诉我们不能直接把结构体当协议。下一步是定义协议整数。C05选择固定宽度无符号整数和big endian写法：最高有效octet先出现。这个选择不是因为本机一定是大端，而是因为协议必须有一个与本机分开的顺序。

## 1. 本机端序和wire order

`std::endian::native`描述当前平台把多字节整数放进对象表示时的顺序。它是环境事实，不是协议规则。wire order是格式自己规定的顺序。一个小端机器也能写大端协议，一个大端机器也能写小端协议；关键是读写双方按同一规则解释字节。

L02把`0x1234`写成两个octet：

```text
12 34
```

这不是从`uint16_t`对象内存里拷出来的结果，而是`append_be<std::uint16_t>`按位移显式取出高8位和低8位。

## 2. 为什么只读无符号整数

公共接口约束是：

```cpp
template<class T>
    requires std::unsigned_integral<T> && (!std::same_as<T, bool>)
std::expected<T, DataError> read_be(std::span<const std::byte> input, std::size_t& cursor);
```

无符号整数的位移和模运算规则更适合拼装octet。`bool`虽然满足某些整数概念，但它不是任意octet容器；协议里的一个字节`0x80`不能自然解释成“合法bool表示”。有符号整数可以在更高层先按无符号读入，再按协议规定检查范围和含义。

## 3. 读大端整数的推导

读`01 02 03 04`为`uint32_t`时，从0开始：

```text
value = 0
value = (0 << 8) | 01 = 0x00000001
value = (1 << 8) | 02 = 0x00000102
value = (0x102 << 8) | 03 = 0x00010203
value = (0x10203 << 8) | 04 = 0x01020304
```

每次左移8位为下一个octet腾位置，再合入低8位。循环次数由`sizeof(T)`决定，所以`uint16_t`读2个octet，`uint32_t`读4个octet，`uint64_t`读8个octet。

## 4. 越界证明：先`cursor <= size`，再`size - cursor`

读取前必须证明缓冲有足够字节。错误写法常见为：

```cpp
if (cursor + width > input.size()) fail();
```

`cursor + width`使用`size_t`，极大值附近会回绕。回绕后结果可能变小，检查反而通过。正确形状是：

```cpp
if (cursor > input.size() || input.size() - cursor < width) fail();
```

第一段先排除`cursor`已经越过末尾；第二段的减法才不会回绕。这个证明方式后续会反复出现：读取payload、跳过未知字段、检查尾随数据都采用同一形状。

## 5. 失败不推进游标

`read_be`成功时按类型宽度推进`cursor`。输入不足时返回`incomplete_input`，并保留原`cursor`。这个保证让调用者可以安全地等待更多输入、记录错误位置或尝试外层恢复。14章会把这个原则扩展到复合字段：只有长度、payload和UTF验证都成功，才提交游标。

## 6. 写大端整数

`append_be`从最高有效octet开始写：

```cpp
for (std::size_t i = sizeof(T); i != 0; --i) {
    out.push_back(static_cast<std::byte>((value >> ((i - 1) * 8)) & 0xff));
}
```

对`uint32_t{0x01020304}`，写出`01 02 03 04`。这里的`0xff`只取最低8位。C05已经用`static_assert(CHAR_BIT == 8)`固定一个octet等于一个C++ byte，否则这个wire format不成立。

## 7. 练习入口与解析

运行L02：

```powershell
cmake -S L02_bits_endian -B build/leaf-L02 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L02 --config Release --parallel 2
ctest --test-dir build/leaf-L02 -C Release --output-on-failure
```

观察程序做三件事：

- `append_be<uint16_t>(0x1234)`后检查前两个octet是`12 34`。
- 连续`read_be<uint16_t>`和`read_be<uint32_t>`，检查值和cursor。
- 从最后一个octet开始读`uint32_t`，检查失败且cursor不变。

第三项是本章最重要的状态契约。解析器失败后消耗输入，会让上层在截断包、流式输入或重试路径里丢失同步位置。

## 8. 自测与解析

**问：既然大多数桌面机器是小端，为什么协议不直接使用本机端序？**

答：协议要跨机器、跨工具和跨版本。把本机端序写进协议会让同一文件在不同机器上解释不同。固定wire order后，本机端序只影响实现方式，不影响格式。

**问：为什么`read_be<int32_t>`不开放？**

答：外部字节先是位模式。用无符号类型接收后，可以显式检查范围，再决定是否映射到有符号含义。这样错误诊断发生在转换前，不会把越界值提前变成实现定义或意外值。

**问：`cursor == input.size()`时读取零字节是否成功？**

答：`read_be<T>`读取固定宽度整数，宽度至少1字节，所以会失败。`take_bytes`这类payload读取如果请求`count == 0`，在`cursor == size`时可以成功返回空片段。整数字段和任意字节片段的契约不同。

**问：`append_be`是否需要看`std::endian::native`？**

答：不需要。它用整数值做位移和mask，生成协议顺序。`std::endian::native`只用于观察本机对象表示，不参与wire format。

## 9. 后续依赖

03章会解释读出一个无符号整数后，怎样把它安全转成更窄类型或业务范围。14章会用`read_be<uint32_t>`读取文本字段长度；如果长度过大或payload不足，必须在分配和访问前拒绝。

