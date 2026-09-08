# 03. Object、符号与静态归档

编译一个翻译单元会得到 object 文件。object 不是可执行程序；它是一组段、重定位记录和符号表。链接器把多个 object 和库组合起来，解析未定义符号，合并段，生成最终可执行文件或库。静态库本质上是 object 成员的归档，链接器按需求抽取成员；这一点决定了缺失定义、链接顺序和“为什么 unused object 里的错误没有触发”。

## object 文件里有什么

一个普通函数编译后会在 object 里留下符号。以 MSVC COFF 为例，`dumpbin /symbols file.obj` 能看到函数名、段号、存储类别等信息。GCC/ELF 路径可用 `nm -C file.o` 观察定义符号、未定义符号和弱符号。

```cpp
int archive_value()
{
    return 42;
}
```

MSVC 符号通常显示为修饰名，例如 `?archive_value@@YAHXZ`，同时工具也会给出可读片段。名字修饰把函数名、命名空间、调用约定和参数类型编码进符号名。C++ 支持重载，所以链接器不能只用源码里的裸函数名。不同 ABI 的修饰规则不同；这也是跨编译器二进制边界必须谨慎的原因。

## 定义、未定义和重定位

如果当前 object 定义了函数，符号表会把它标成某个段里的外部定义。如果当前 object 只是调用了函数但没有定义，符号会是未定义引用，链接器必须从其他 object 或库里找到定义。

声明只告诉编译器“有这样一个函数，可以生成调用代码”。定义才提供函数体。缺失定义的典型诊断在 MSVC 是 `LNK2019 unresolved external symbol`，在 GNU 工具链常见 `undefined reference`。这些是链接阶段问题，不应被混成编译错误或 ODR 重复定义。

## 静态库是 object 成员归档

`lib.exe` 或 `ar` 生成的静态库不是运行时加载的 DLL/so，而是一个索引加若干 object 成员。链接器处理静态库时，会根据当前 unresolved symbol 集合选择需要的成员。被抽取的成员进入链接；未被抽取的成员通常不参与解析它自己的未定义符号。

C2 的例子故意把库分成两个成员：

- `used_member.cpp` 定义 `archive_value()`，主程序会调用它。
- `unused_member.cpp` 定义 `unused_member_value()`，内部调用一个没有定义的 `missing_unused_dependency()`。

主程序只需要 `archive_value()` 时，链接器从库中抽取 `used_member.obj`，不会抽取 `unused_member.obj`，所以程序能链接运行。如果把两个 object 直接放到链接命令，`unused_member.obj` 也参与链接，它内部的缺失定义会触发链接失败。这个实验说明“静态库成员按需抽取”不是“库里所有代码都已经完整正确”。

## 链接顺序：COFF 与 ELF 不要互相套结论

GNU/ELF 链接器传统上按命令行从左到右扫描静态库：对象或库先产生未解析符号，后面的库再满足它；如果库出现在需求之前，可能不会被重新扫描，需要调整顺序、使用 group 或改变构建组织。

MSVC/COFF 的 `link.exe` 对库索引和默认库处理方式不同，常见情况下不会表现为同一套简单的左到右一次扫描规则。课程不能把 MSVC 的库重扫现象套到 GNU，也不能把 GNU 顺序问题写成所有平台一致。C2 在 Windows 上实测 COFF 的 object、lib 和 link 行为；ELF 的 `nm`/`ar` 命令和示例代码保留，但没有本机验证时标为未验证。

## 模板显式实例化的工程规则

模板定义通常要在使用点可见，因为编译器需要用具体模板实参生成代码。把模板函数定义只放进 `.cpp`，其他翻译单元只看到声明，通常会导致缺失实例化的链接错误。

一种工程做法是显式实例化：

```cpp
// box.hpp
template <class T>
T twice(T value);

// box.cpp
template <class T>
T twice(T value) { return value + value; }

template int twice<int>(int);
```

这样 `box.cpp` 明确生成 `twice<int>` 的定义，其他翻译单元可以只声明并调用 `twice<int>`。边界很清楚：你只承诺列出的实例化。调用 `twice<double>` 仍需要定义可见或另一个显式实例化。完整模板查找、偏序和约束放到 C04；C01 只要求理解“实例化产物也是符号，也受链接和 ODR 约束”。

## C2 练习如何验证

C2 的 Reference 构建一个静态库并运行消费者，证明按需抽取的 `used_member` 可以提供 `archive_value()`。Observation 使用 `dumpbin /symbols` 和 `dumpbin /linkermember` 检查 object 与 archive 里的目标符号。Negative 子工程先确认 object 编译成功，再确认链接阶段失败，分别覆盖缺失定义和“直接链接所有 object 会暴露 unused member 的缺失依赖”。

所有 negative 检查都拒绝 configure 失败、编译失败和 timeout。失败必须发生在 link target，且诊断要匹配目标符号。这样才能证明我们测的是链接模型，而不是随便找了一个失败命令。
