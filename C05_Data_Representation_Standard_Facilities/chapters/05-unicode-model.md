# 05：Unicode单位模型：不要把“一个字符”当规格

“一个字符”在工程里太模糊。它可能指一个byte、一个UTF-8 code unit、一个UTF-16 code unit、一个code point、一个Unicode scalar value，也可能指用户眼里的一整个grapheme cluster。C05后续要报告错误offset、限制字段长度、转码、规范化和切分显示文本，所以必须先把单位拆开。

## 1. Code point和scalar value

Unicode code point是`U+0000`到`U+10FFFF`范围内的编号。Unicode scalar value是除去UTF-16代理区`U+D800`到`U+DFFF`后的code point。UTF-8和UTF-16合法文本编码的是scalar value，不编码孤立代理项。

例子：

- `U+0041`是字符A，也是scalar value。
- `U+20AC`是欧元符号，也是scalar value。
- `U+1F642`是一个补充平面的scalar value。
- `U+D800`是代理区code point，不是scalar value，不能作为合法UTF-8文本的结果。

noncharacter也是code point和scalar value的一部分。本课严格UTF验证不因为某个scalar value是noncharacter就拒绝它；是否允许这类值属于更上层的数据策略。

## 2. UTF-8 code unit

UTF-8的code unit是8-bit byte。一个scalar value编码成1到4个byte：

| 范围 | UTF-8长度 |
|---|---|
| U+0000..U+007F | 1 byte |
| U+0080..U+07FF | 2 bytes |
| U+0800..U+FFFF | 3 bytes，但不包括代理区 |
| U+10000..U+10FFFF | 4 bytes |

严格UTF-8还要求最短编码。`/`是`U+002F`，合法UTF-8是`2F`；写成`C0 AF`就是overlong，必须拒绝。拒绝overlong不是吹毛求疵：如果某层把`2F`当路径分隔符拦住，另一层又把`C0 AF`宽松解码成`/`，边界就被绕过。

## 3. UTF-16 code unit和代理对

UTF-16的code unit是16-bit。基本多文种平面里非代理区的scalar value使用一个code unit；补充平面使用一对代理项：

```text
U+1F642 -> high surrogate D83D, low surrogate DE42
```

高代理范围是`D800..DBFF`，低代理范围是`DC00..DFFF`。高代理后面没有低代理，或者低代理单独出现，都是非法UTF-16输入。错误offset用UTF-16 code unit下标，而不是byte下标，也不是code point下标。

## 4. Grapheme cluster是显示层单位

用户看到的“一个字符”常常是多个code point组合。例如一个基础字母加组合重音、区域指示符组成旗帜、emoji加肤色修饰符或ZWJ序列。它们的显示边界由Unicode文本分割规则决定，不等于UTF-8 byte数，也不等于UTF-16 code unit数。

C05在05和06章只处理编码合法性和转码。规范化、大小写折叠和grapheme segmentation放到07章及ICU扩展。不能因为06章能把UTF-8转成UTF-16，就声称已经能按用户感知字符正确截断或计数。

## 5. BOM是码点，也是格式规则

`U+FEFF`可以作为字节序标记。对于UTF-8，开头字节`EF BB BF`常被当作BOM；对于通用转码函数，它仍然是一个合法scalar value。本课`utf8_to_utf16`保留它。某个文本文件格式是否把开头BOM剥掉，是上层解析规则。配置解析会定义自己的BOM处理，UTF转换层不偷做。

## 6. 练习入口与解析

运行L05：

```powershell
cmake -S L05_unicode -B build/leaf-L05 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L05 --config Release --parallel 2
ctest --test-dir build/leaf-L05 -C Release --output-on-failure
```

观察程序检查：

- `U+20AC`在UTF-8中占3个byte，在UTF-16中占1个code unit。
- `U+1F642`在UTF-8中占4个byte，在UTF-16中占2个code unit。
- 通用转码保留开头BOM。

这些断言都来自`c05::utf8_to_utf16`，它只证明编码转换和单位关系，不证明规范化、排序、大小写、显示宽度或grapheme切分。

## 7. 自测与解析

**问：`std::string::size()`能表示Unicode字符数吗？**

答：不能。对UTF-8文本，它返回byte数。`"€"`的UTF-8长度是3，`"🙂"`的UTF-8长度是4。它不等于code point数，更不等于用户感知字符数。

**问：UTF-16里`u16string::size()`能表示code point数吗？**

答：不能。它返回UTF-16 code unit数。补充平面scalar value需要两个code unit。`U+1F642`的`u16string::size()`为2，但它对应一个scalar value。

**问：代理项为什么在UTF-8里非法？**

答：代理项是UTF-16内部用来表示补充平面的机制，不是Unicode scalar value。UTF-8直接编码scalar value，因此不能编码`U+D800..U+DFFF`。

**问：noncharacter是否应该被UTF验证拒绝？**

答：本课通用UTF验证不拒绝。noncharacter不是代理项，也不超出`U+10FFFF`。如果某个业务协议不允许它，应在业务层添加规则，并写清诊断。

**问：规范化能不能放进06章转码里顺手做？**

答：不应该。转码只改变编码形式，不改变文本等价类。规范化可能改变code point序列，例如组合字符和预组合字符之间转换。它是文本语义层，放到07章。

## 8. 后续依赖

06章会把这些单位落实到代码：UTF-8错误offset按byte报告，UTF-16错误offset按code unit报告。14章会把payload内byte offset映射到整个输入buffer的byte offset。

