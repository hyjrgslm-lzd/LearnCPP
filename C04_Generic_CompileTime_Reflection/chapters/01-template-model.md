# 01. 模板模型：从一份算法到一族类型和函数

模板解决的问题不是“少写几份代码”这么简单。真正重要的是把一组类型或常量差异放进接口，让编译器在实际使用点生成对应的函数、类或变量，同时仍然检查每个生成版本是否满足 C++ 的普通规则。模板不是运行时分派，也不是文本替换。它有自己的声明、推导、实例化和链接模型；不理解这些边界，后面的重载、约束、CPO、反射和编译成本都会变成经验写法。

先看一个普通函数：

```cpp
int to_base_kilometers(int value) {
    return value * 1000;
}
```

它只处理一种单位和一种值类型。如果又要处理 `long long`、`double`、米、厘米，直接复制函数会把“单位换算”这个规则散落到许多地方。模板的第一步是把会变化的部分变成参数：

```cpp
struct kilometers {};

template<class T, class Unit>
struct quantity {
    T value{};
};

template<class Unit>
inline constexpr int unit_scale_v = 0;

template<>
inline constexpr int unit_scale_v<kilometers> = 1000;

template<class T, class Unit>
constexpr auto to_base(quantity<T, Unit> q) {
    return q.value * unit_scale_v<Unit>;
}
```

这里同时出现了类模板、变量模板、显式特化和函数模板。`quantity<T, Unit>` 是一族类型的模板声明；`quantity<int, kilometers>` 才是某个具体类型。`unit_scale_v<Unit>` 是一族变量，主模板给默认倍率，`unit_scale_v<kilometers>` 是对一个完整实参列表的显式特化。`to_base` 是函数模板，编译器在调用 `to_base(quantity<int, kilometers>{2})` 时推导 `T=int, Unit=kilometers`，然后实例化出一个可调用函数。

这个模型有两层时间。第一层是模板定义本身被解析。编译器必须能理解不依赖模板参数的名字和语法。第二层是模板被具体实参使用时实例化，依赖模板参数的表达式才用实际类型检查。这个分层常被叫作两阶段处理。它解释了一个反直觉现象：模板定义里可以写下某些暂时无法完全检查的表达式，但只要没有被实例化到那个成员或函数体，就不会立刻报错。

```cpp
template<class T>
struct lazy_box {
    T value;

    auto only_for_pointer() const {
        return *value;
    }
};

lazy_box<int> ok{1}; // 只形成 lazy_box<int>，没有调用 only_for_pointer()
```

`lazy_box<int>` 这个类型可以存在，因为 `only_for_pointer` 的函数体按需实例化。真正调用 `ok.only_for_pointer()` 时，`*int` 不合法，错误才出现。按需实例化不是“错误被忽略”，而是“还没有要求生成那部分代码”。练习的 observation 会用一个不会调用的成员展示这条边界，再用一个受控调用展示错误应该出现在哪里。

函数模板也按使用点实例化。不同实参会生成不同的 specialization。这里的 specialization 是标准术语，表示“某个模板实参列表对应的实体”，不一定是你手写的 `template<>`。手写 `template<>` 是显式特化；编译器按调用生成的是隐式实例化。类模板还可以写偏特化：

```cpp
template<class A, class B>
struct same_unit : std::false_type {};

template<class T>
struct same_unit<T, T> : std::true_type {};
```

显式特化要求给出完整实参列表，比如 `unit_scale_v<kilometers>`。偏特化只适用于类模板或变量模板，保留一部分模式让编译器匹配，比如“两个实参是同一个类型”。函数模板没有偏特化；函数族的选择靠重载和偏序，后面会单独讲。

别名模板不生成新类型，它只是按实参展开成另一个类型表达式：

```cpp
template<class Q>
using quantity_value_t = typename Q::value_type;
```

这行代码的 `typename` 是必要的，因为 `Q::value_type` 依赖模板参数，编译器在定义阶段不知道它是类型还是静态成员。这里先接受写法，依赖名和 `typename` 会在名字查找章节深讲。

模板还会碰到 ODR 和多翻译单元问题。一个函数模板定义通常放在头文件，因为每个使用它的翻译单元都需要看到函数体才能实例化。类模板成员同理。若只把模板声明放头文件、定义藏在 `.cpp`，其他翻译单元通常无法为自己的实参生成代码。显式实例化可以把某组实参的生成集中到一个 `.cpp`：

```cpp
// header
template<class T>
T twice(T);

extern template int twice<int>(int);

// one .cpp
template<class T>
T twice(T x) { return x + x; }

template int twice<int>(int);
```

`extern template` 的意思是“这个翻译单元不要隐式实例化这组实参，别处会提供定义”。它减少重复实例化和目标文件膨胀，但只适合你明确知道要集中提供哪些实参。漏掉显式实例化会变成链接错误；多个翻译单元各自提供不一致定义则触碰 ODR。ODR 的坏例子不适合依赖“必然报错”来教学，因为跨翻译单元不一致常属于 ill-formed, no diagnostic required；本课用正例证明“唯一提供者”边界，用漏提供者的隔离链接负例证明 `extern template` 不是语法检查问题。C01 已经讲过翻译单元和链接，本章只强调模板给这件事增加了“生成点”的问题。

模板的正确性边界也要说清。模板声明不代表所有 `T` 都能工作。`to_base` 要求 `q.value * unit_scale_v<Unit>` 良构，结果可返回。`add_same_unit` 要求两个 quantity 的 unit 相同。早期代码常把这些前提藏在函数体里，等实例化失败时才吐出长诊断；现代 C++ 更倾向于用 requires 把接口边界写出来。本课后面会系统讲 Concepts，这里先让练习 checker 用 `requires` 检查你暴露出的行为。

练习 L01 要你实现一个很小的 `quantity` 模板族。它不是业务单位库，只覆盖本章机制：类模板保存值和标签；变量模板给单位倍率，显式特化覆盖厘米、米和公里；别名模板提取值类型；偏特化判断两个 quantity 是否同单位；函数模板只允许同单位相加，并把单位值换算到以厘米为整数基准的基础单位。Student 起点会编译，但它把所有单位倍率写成 1，`quantity_value_t` 不完整，同单位判断过宽，checker 会拒绝这些假完成。完整答案不需要任何框架，也不需要宏。

读完本章，你应该能区分几件事：模板定义何时被解析，具体实体何时被实例化；隐式实例化、显式特化、偏特化和显式实例化分别解决什么问题；为什么模板定义通常要在头文件；为什么 `extern template` 是工程成本工具，不是语义魔法。后面的推导、转发和重载都建立在这条模型上。

自测：如果一个类模板成员函数体里写了 `T::missing()`，只声明 `Box<int>* p = nullptr;` 会不会报错？不会，因为没有实例化那个成员函数体。若调用 `Box<int>{}.f()`，就会在实例化 `f<int>` 时检查 `int::missing` 并报错。另一个自测：`template<>` 和偏特化谁能用于函数模板？函数模板能显式特化，但不能偏特化；要表达“部分模式”的函数选择，用重载、约束和偏序。
