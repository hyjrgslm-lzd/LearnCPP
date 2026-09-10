# 06：严格UTF验证与转码

有了Unicode单位模型，还需要可执行规则。C05的UTF函数采用严格策略：合法就转换，非法就返回`DataError`，不把坏字节替换成`U+FFFD`后继续成功。这个选择服务于资源清单包：名称、路径和note都是标识性数据，静默替换会让不同坏输入坍缩成同一个字符串，错误位置也丢失。

## 1. UTF-8验证规则

`validate_utf8(std::string_view)`从左到右读取一个UTF-8序列。首字节决定长度：

- `00..7F`：单字节ASCII。
- `C2..DF`：两字节序列。
- `E0..EF`：三字节序列。
- `F0..F4`：四字节序列。

其他首字节直接非法。`80..BF`不能单独作为首字节；`C0`和`C1`会形成overlong；`F5..FF`超过Unicode最大范围。

后续字节必须满足`10xxxxxx`。如果输入在需要的后续字节前结束，报告`incomplete_input`；如果后续字节形状不对，报告`invalid_encoding`。错误offset使用坏序列起始byte，而不是已经读到的那个坏continuation的位置。这样调用者能指向“这个UTF-8序列从哪里开始坏”。

解出code point后还要检查：

- 是否使用最短编码，拒绝overlong。
- 是否落入`U+D800..U+DFFF`代理区，拒绝。
- 是否超过`U+10FFFF`，拒绝。

## 2. UTF-8到UTF-16

`utf8_to_utf16`先按UTF-8严格解码为scalar value，再写UTF-16：

- `cp <= 0xFFFF`时写一个`char16_t`，但代理区不会到这里，因为前面已拒绝。
- `cp > 0xFFFF`时减去`0x10000`，高10位写入高代理，低10位写入低代理。

输出是拥有型`std::u16string`。输入`std::string_view`只在调用期间借用。NUL作为`U+0000`保留，BOM作为`U+FEFF`保留。

## 3. UTF-16到UTF-8

`utf16_to_utf8`读取UTF-16 code unit：

- 普通非代理code unit直接形成同值scalar value。
- 高代理必须后接低代理，两者组合成补充平面scalar value。
- 高代理到输入末尾，报告`incomplete_input`。
- 高代理后不是低代理，报告`invalid_encoding`。
- 低代理单独出现，报告`invalid_encoding`。

错误offset单位是`utf16_code_unit`。例如`{0xD83D}`失败在code unit 0；`{0xD83D, 0x0041}`也报告起始高代理的位置0，而不是'A'的位置1。当前公共实现对“高代理后接非低代理”按起始高代理位置报告，便于定位坏代理对。

## 4. 资源预算

通用转换有1MiB预算：

- UTF-8输入`size()`不能超过`max_package_bytes`。
- UTF-16输入按字节预算，先检查`size() <= max_package_bytes / sizeof(char16_t)`。
- 输出追加前检查是否会超过对应预算。

输入检查防止接收超大外部数据。输出检查防止合法输入在转换后膨胀超过包预算。例如大量补充平面字符从UTF-16转UTF-8会从2个code unit变成4个UTF-8 byte；大量ASCII从UTF-8转UTF-16会从1 byte变成2 byte。预算必须覆盖输出，而不是只看输入。

## 5. 错误位置映射

UTF函数只知道自己的输入视图。`validate_utf8("x\xe2\x28\xa1")`会报告offset 1，因为坏序列从payload内byte 1开始。14章读取字段时，payload可能从整个buffer的byte 4或更后开始，因此外层要把payload内offset加上payload起点，得到整个buffer的byte offset。

不要混用单位：

- UTF-8验证和UTF-8输入限制用byte offset。
- UTF-16输入错误和UTF-16输入限制用UTF-16 code unit offset。
- code point计数和grapheme计数都不是本章诊断单位。

## 6. `char8_t`入口

练习的Reference给了便捷重载：

```cpp
inline std::expected<std::u16string, c05::DataError> utf8_to_utf16(const char8_t* s)
{
    return c05::utf8_to_utf16(std::string_view(reinterpret_cast<const char*>(s)));
}
```

这个重载服务于源码字面量，例如`u8"z¢€🙂"`。它没有长度参数，所以只适合NUL终止的字面量类输入；包含嵌入NUL的外部数据应使用`std::string_view`或`const char*, size_t`形式。L06检查器用`utf8_to_utf16("A\0B", 3)`确认嵌入NUL被保留。

## 7. 练习入口与解析

运行L06：

```powershell
cmake -S L06_transcoding -B build/leaf-L06 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L06 --config Release --parallel 2
ctest --test-dir build/leaf-L06 -C Release --output-on-failure
```

学生只改：

```text
L06_transcoding/src/student/utf_transcode.hpp
```

必须实现：

| Part | 要求 | 解析 |
|---|---|---|
| UTF-8验证 | 拒绝overlong、代理项、超范围、坏continuation、截断 | 首字节范围和解码后范围都要查；只查continuation不够 |
| UTF-8到UTF-16 | 输出拥有型`u16string`，保留NUL/BOM | 转码层不做Manifest业务过滤 |
| UTF-16到UTF-8 | 正确处理代理对和孤立代理 | 错误offset按UTF-16 code unit |
| 预算 | 输入和输出都受1MiB限制 | 转换可能膨胀，不能只检查输入 |

`validation/bad`漏掉UTF-8验证，检查器用`"C0 AF"`这类overlong输入拒绝它。`bad_rejected`通过只说明检查器抓到了代表性错误。

## 8. 自测与解析

**问：为什么坏continuation的offset报告序列起点，而不是坏字节位置？**

答：诊断目标是“这个UTF-8序列非法”。序列从首字节开始，调用者通常需要从那里重新同步或标记整段坏编码。报告起点也和截断、overlong、代理项等错误保持一致。

**问：为什么不自动替换成`U+FFFD`？**

答：替换是有损恢复策略。资源名称和路径是标识，静默替换可能制造碰撞，也会隐藏上游数据损坏。本课严格转换把策略留给调用者：要恢复时另写一个明确的宽松接口。

**问：BOM为什么不在`utf8_to_utf16`里剥掉？**

答：BOM处理属于文本格式。通用转码只变换编码，保留`U+FEFF`。配置文件解析可以在“文件开头”这个上下文里处理BOM；普通字符串中间出现`U+FEFF`时，转码层没有资格删它。

**问：UTF-16输入限制为什么按`max_package_bytes / sizeof(char16_t)`检查？**

答：包预算以byte为单位。`u16string::size()`数的是code unit，不是byte。先除以`sizeof(char16_t)`，才能判断输入底层字节量是否超过预算，并避免乘法溢出。

**问：如果UTF-8输入合法，输出UTF-16还可能超预算吗？**

答：可能。ASCII每个输入byte会变成一个UTF-16 code unit，也就是通常2个byte。接近1MiB的ASCII输入转换后会超过1MiB输出预算。追加前检查输出容量是必要规则。

## 9. 后续依赖

14章会调用`validate_utf8`验证长度前缀字段，并把payload内byte offset映射到整个buffer。07章会在严格合法文本之上讨论规范化、大小写和grapheme；ICU扩展也必须先通过严格验证，避免库的宽松替换行为掩盖坏输入。

