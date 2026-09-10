# 01：字节、对象表示与不能偷懒的序列化

C++对象有类型和值，也有object representation。object representation是对象占用的`unsigned char`序列；它能被复制、保存和检查，但它不是自动稳定的外部格式。C05从这里开始，是为了拆掉一个常见错觉：既然对象在内存里也是字节，那把结构体写进文件就等于序列化。

这个错觉在单机demo里经常看起来能跑。结构体字段刚好排列紧凑，本机读回同一个编译器写出的文件，值也许对。但协议要求的是跨编译器、跨版本、跨机器、跨进程和跨时间的解释一致。C++对象表示没有给你这些承诺。

## 1. 三个不同问题

先分清三件事：

- 对象的值是什么，例如`PacketWord{0x1234, 0x00ff}`。
- 这个对象在当前实现里的字节表示是什么，例如`sizeof(PacketWord)`个`std::byte`。
- 外部格式规定的字节序列是什么，例如“tag占2个octet，大端，flags占2个octet，大端”。

第一件事属于C++值语义。第二件事属于当前实现里的对象表示。第三件事属于协议。`std::bit_cast`只能在第一和第二之间复制表示，不能替你定义第三件事。

## 2. `std::byte`、`char`和`unsigned char`

`std::byte`表示原始字节。它不是字符，也不隐式参与整数运算。需要看数值时，用`std::to_integer<unsigned char>(b)`显式转换。这个麻烦是好事：它阻止你把“字节容器”和“文本字符”混成一个概念。

`char`常用于`std::string`，但`std::string`本质是拥有一串`char`对象，不保证它保存的是合法文本。`unsigned char`和`std::byte`适合表达原始存储或协议octet。C05的wire format要求`CHAR_BIT == 8`，也就是一个byte正好有8 bit；这是课程格式的显式前提，不是C++对所有目标平台的普遍保证。

## 3. `std::bit_cast`合法做什么

`std::bit_cast<To>(from)`要求源类型和目标类型大小相同，且两边都是trivially copyable。它按对象表示复制位模式，返回一个新的`To`对象。它不产生指向原对象的别名，也不延长任何对象生命期。

L01里的核心观察是：

```cpp
PacketWord word{0x1234, 0x00ff};
auto bytes = std::bit_cast<std::array<std::byte, sizeof(PacketWord)>>(word);
auto round_trip = std::bit_cast<PacketWord>(bytes);
```

往返后值保持，是因为同一个程序、同一个类型、同一份表示被复制回去。随后修改`bytes[0]`再`bit_cast`回`PacketWord`，检查到值变化。这个实验说明“当前表示参与了当前值的重建”，不说明这份bytes可以作为跨平台文件格式。

## 4. padding不是你的字段

结构体可能包含padding。padding用于满足对齐或布局要求，不是你声明的字段。两个值相等的对象，padding字节也不一定相同；读取未初始化padding得到的具体字节不能当成业务数据。即使`PacketWord`在当前编译器上看起来没有padding，也不能把这个观察推广给所有结构体。

协议里应该写字段，不应该写padding。C05后续的Manifest字段会显式写：

```text
tag:u16 | length:u32 | payload bytes
```

这里每个整数宽度、端序和payload长度都是格式的一部分。没有“结构体中间刚好多出来几个字节”的隐含字段。

## 5. 别名、对齐和生命期边界

把`std::byte*`或`char*`强转成`PacketWord*`再读，看起来比逐字段解析少写代码，但它同时押注几件事：地址对齐满足`PacketWord`，那段存储里真的有一个`PacketWord`对象处于生命期内，读取类型没有违反别名规则，本机端序和协议一致，padding内容也按你想的排列。

这些条件通常都不是外部输入能保证的。C05用`std::span<const std::byte>`表示外部缓冲，用`read_be<T>`逐octet组装整数。这样做代码稍长，但每个前提都在格式里显式出现，错误也能定位。

## 6. 对象表示可以用于什么

对象表示不是没用。它适合：

- 观察当前实现的布局现象。
- 对trivially copyable对象做安全的表示复制。
- 实现哈希、诊断dump或低层工具时保留“当前进程里的表示”。

它不适合直接作为长期文件格式、网络协议、跨语言ABI或版本兼容格式。只要数据要离开当前程序边界，就应该写协议字段，读协议字段，再构造C++对象。

## 7. 练习入口与解析

运行L01：

```powershell
cmake -S L01_bytes -B build/leaf-L01 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L01 --config Release --parallel 2
ctest --test-dir build/leaf-L01 -C Release --output-on-failure
```

L01是观察题，不需要编辑。你要解释三个断言：

| 断言 | 含义 |
|---|---|
| `bit_cast`往返保持值 | 同一类型表示复制回同一类型，值可重建 |
| byte数组长度等于`sizeof(PacketWord)` | 对象表示覆盖对象占用的全部字节 |
| 修改一个byte后值变化 | 当前类型的某些表示位影响当前值 |

第三条不是跨机器协议证据。它没有固定字节序，没有排除padding差异，也没有说明其他编译器会得到同一字节序列。

## 8. 自测与解析

**问：为什么不能用`sizeof(MyHeader)`作为文件头长度并直接写结构体？**

答：`sizeof`包含当前实现为了布局加入的padding；字段顺序、对齐和整数端序也没有变成协议承诺。下一版编译器或另一个语言实现可能布局不同。文件头应该显式规定每个字段的宽度和端序。

**问：`bit_cast`是不是比`reinterpret_cast`更适合解析网络包？**

答：`bit_cast`比把指针强转后解引用更容易避开别名和对齐问题，但它仍然只复制表示。它不检查输入长度，不处理协议端序，不验证字段范围，也不能让padding变成稳定格式。解析网络包仍应逐字段读取。

**问：`std::string`能不能当字节数组？**

答：它能拥有一串`char`，可以保存任意`char`值，包括NUL。但把它叫“字符串”容易让人误以为内容是合法文本。C05对原始协议输入使用`std::byte`，对已经验证或准备验证的UTF-8字节使用`std::string_view`或`std::string`。

**问：L01修改byte后再转回对象，会不会触发未定义行为？**

答：示例选用简单的trivially copyable结构体，修改表示后`bit_cast`回同类型。这个观察用于说明表示位会影响值。对某些类型，任意位模式可能不是合法值；本课不会把这种实验推广为“随便改任意对象表示都安全”。

## 9. 后续依赖

02章会在“不能直接写对象表示”的基础上，定义怎样用位移和mask写出固定大端整数。14章会把这个能力用于读取`u32be length`，再进入UTF-8文本字段。

