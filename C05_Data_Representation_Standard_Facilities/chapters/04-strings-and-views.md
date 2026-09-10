# 04：`std::string`、`std::string_view`与借用边界

C++里的`std::string`不是“已经验证的自然语言文本”，它是拥有型`char`序列。`std::string_view`也不是轻量字符串对象，它是不拥有的视图。C05把字符串放在Unicode之前讲，是为了先解决所有权和边界：谁拥有字节，谁只是借用，什么时候结果必须拷贝，NUL是否结束内容，`char8_t`怎样进入字节级UTF解析器。

## 1. `std::string`拥有字节

`std::string`管理一段连续`char`存储，长度由`size()`记录。它可以包含嵌入的NUL：

```cpp
std::string s("A\0B", 3);
```

此时`s.size() == 3`。如果把`s.c_str()`交给只认C字符串的接口，很多函数会在第一个NUL停止，看到的只是`"A"`。C05所有协议长度都用显式长度，不用NUL终止推断字段大小。

## 2. `std::string_view`借用字节

`std::string_view`保存指针和长度，不拥有存储。它可以指向`std::string`、字符串字面量、缓冲区的一段，也可以在底层对象销毁后变成悬垂视图。函数接收`string_view`通常很好，因为它说明函数只在调用期间读取输入：

```cpp
std::expected<void, DataError> validate_utf8(std::string_view s);
```

但函数如果要返回一个会离开当前调用边界的文本，应该返回拥有型对象：

```cpp
std::expected<std::string, DataError> read_utf8_field(...);
```

返回`string_view`会把输入缓冲生命期责任推给调用者。这个设计不是错，但本课主线选择拥有型输出，让后续Manifest对象不依赖原始包缓冲继续存活。

## 3. 视图会观察到拥有者变化

L04里：

```cpp
std::string text = "config";
std::string_view view = text;
auto owned = owned_upper_ascii(view);
text[0] = 'C';
```

`view`之后看到`"Config"`，因为它仍指向`text`的存储。`owned`保持`"CONFIG"`，因为函数返回了独立`std::string`。这就是“入参借用、出参拥有”的基本边界。

如果`text`发生重新分配，例如追加大量内容，旧`view`可能悬垂。观察程序没有展示这条危险路径，因为课程不需要靠未定义行为来证明借用规则；只要理解view不拥有，就能推导出这个风险。

## 4. `char8_t`说明编码意图，不证明合法性

C++20引入`char8_t`，`u8"..."`的元素类型是`char8_t`。这能在类型层表达“这些字面量按UTF-8编码”。但`std::u8string`仍然只是一串UTF-8 code unit；如果数据来自外部文件、网络或`reinterpret_cast`，类型本身不验证它是否合法。

C05的UTF函数接收`std::string_view`，即字节级UTF-8视图。L04使用：

```cpp
std::u8string u8 = u8"path";
std::string_view bridge(reinterpret_cast<const char*>(u8.data()), u8.size());
```

这个桥接只把`char8_t`存储交给字节级parser读取。它不改变字节，不创建文本保证，也不允许你忽略06章的严格验证。对字面量`u8"path"`，合法性来自源代码字面量规则；对外部输入，合法性必须运行时检查。

## 5. 不依赖NUL，不偷换编码层

协议字段常见错误是用`strlen`计算UTF-8 payload长度。只要文本包含NUL，长度就错；只要文本包含多字节字符，“字符数”和byte数也不同。C05所有字段长度都数payload byte。UTF-8合法性由06章验证，业务上是否允许NUL由Manifest字段规则决定。

同理，`std::string::size()`返回的是`char`数量，也就是本课UTF-8存储里的byte数量，不是Unicode code point数量，也不是用户看到的字素数量。05章会展开这些单位。

## 6. 练习入口与解析

运行L04：

```powershell
cmake -S L04_strings_views -B build/leaf-L04 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L04 --config Release --parallel 2
ctest --test-dir build/leaf-L04 -C Release --output-on-failure
```

L04是观察题。三个Part对应三条规则：

| Part | 观察 | 解析 |
|---|---|---|
| 1 | view等于原始`"config"` | `string_view`指向现有存储 |
| 2 | 修改owner后view变，owned不变 | 借用不拥有；返回跨边界值要拥有 |
| 3 | `char8_t`桥接到`string_view` | 类型桥接不是UTF验证 |

## 7. 自测与解析

**问：函数参数用`std::string_view`是不是总比`const std::string&`好？**

答：当函数只同步读取字符序列且不保存视图时，`string_view`通常更通用。它能接收`string`、字面量和片段。但如果函数需要NUL结尾、需要修改字符串、需要保存引用，或者调用异步任务在函数返回后使用输入，`string_view`就不合适。

**问：`std::string_view`能不能返回局部`std::string`的一段？**

答：不能。局部`std::string`在函数返回时销毁，返回的view会悬垂。返回拥有型`std::string`，或让调用者提供并拥有底层存储。

**问：`std::string`包含NUL时还能作为UTF-8吗？**

答：可以。U+0000是合法Unicode scalar value，UTF-8编码为单个0 byte。通用UTF转换保留它。某些业务格式可以额外拒绝NUL，例如Manifest名称和路径；那是上层规则。

**问：`char8_t`为什么还要`reinterpret_cast`到`char`？**

答：本课UTF parser以`std::string_view`接收字节，元素类型是`char`。`char8_t`和`char`是不同类型，所以需要显式桥接。桥接只改变访问接口，不验证编码、不改变所有权。

## 8. 后续依赖

05章会把“字节长度”和Unicode单位分开。06章会在`string_view`输入上执行严格UTF-8验证，并返回拥有型`u16string`或`string`。14章读取payload后会构造拥有型`std::string`，再验证UTF-8，成功后才提交游标。

