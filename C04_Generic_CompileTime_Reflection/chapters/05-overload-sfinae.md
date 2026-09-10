# 05. 重载决议、偏序与 SFINAE

名字查找只回答“有哪些声明可能参与”。真正调用哪一个，还要经过重载决议。泛型代码里，这一步会同时处理普通函数、函数模板、约束、转换序列和替换失败。若只记一句“更特化的优先”，很容易在实际接口里写出两个都可行、排序不稳定或错误在函数体里炸开的重载。

本章从一个很小的分类接口开始：`c04_overload::describe(x)`。它把输入分成成员自描述、字符串字面量、整数和 range 四类。这个接口足够小，但能覆盖候选集、可行函数、转换等级、函数模板偏序、SFINAE 边界，以及“函数不能偏特化”的替代写法。

## 候选集和可行性

调用 `f(arg)` 时，查找阶段给出候选声明。随后编译器检查每个候选能否用这些实参调用。参数数量不对、约束不满足、替换失败的模板不会进入可行集合。可行集合里再比较转换序列。

```cpp
void f(long);
void f(const char*);

f(1);      // int 到 long，标准整数转换
f("abc");  // 数组到指针退化，再匹配 const char*
```

`"abc"` 的类型不是 `const char*`，而是 `const char[4]` 左值。函数形参若写 `const char*`，数组到指针退化会发生，长度信息丢掉。若你要保留字面量长度，应该写数组引用：

```cpp
template<std::size_t N>
void g(const char (&literal)[N]);
```

练习 L05 的一个核心检查就是这个。bad 控制实现把字符串字面量当 `const char*`，能跑出结果，但已经丢失 `N`。checker 要求 `"abc"` 返回 `literal_result<4>`，用类型结果证明没有退化。

## 转换等级与非模板优先的前提

重载决议比较转换序列时，大致按 exact match、promotion、conversion、user-defined conversion、ellipsis 排序。更好的转换胜出。若两个候选转换同样好，才进入额外规则：非模板函数通常优先于模板函数。

这个“通常”有前提：非模板候选必须已经可行，且转换不比模板差。

```cpp
void h(long);

template<class T>
void h(T);

h(1); // 模板 h<int> 是 exact match，h(long) 需要转换；模板胜出
```

所以“非模板优先”不是绝对口号。它不是压过更差转换的特权。设计泛型接口时，不要靠非模板兜底去赌排序；把你真正要的域写进参数和约束里。

## 函数模板偏序

当两个函数模板都可行，转换又无法直接分出胜负时，编译器会做函数模板偏序。它尝试判断一个模板是否比另一个接受更窄的集合。

```cpp
template<class T>
void use(T&);

template<class T>
void use(const T&);
```

对非常量左值，`T&` 更贴近；对 const 左值，两个形态的推导结果不同，最终排序按规则决定。真实接口里，偏序常和约束一起出现。若一个重载写 `std::ranges::range<T>`，另一个写 `std::ranges::forward_range<T>`，后者语义上更窄，但排序是否能利用这种“更窄”取决于约束是否形成可识别的包含关系。第 06 章会专门讲原子约束身份。

L05 里我们不让读者靠复杂偏序猜结果。`describe` 的路径按约束拆开：成员描述优先；字符串字面量单独重载，避免退化；整数排除 `bool`；range 只在前面几类不成立时成立。这比写一个大模板再在函数体里分支更好诊断。

## 类模板可以偏特化，函数模板不能

类模板偏特化是合法工具：

```cpp
template<class T>
struct box;

template<class T>
struct box<T*> {};
```

函数模板没有偏特化。下面这种想法不成立：

```cpp
template<class T>
void f(T);

template<class T>
void f<T*>(T*);
```

函数的替代方案是重载。你可以写 `template<class T> void f(T*)`，让重载决议和函数模板偏序选择它。也可以把选择逻辑放到类模板 traits 里，用类模板偏特化算出类型，再由一个函数调用 traits。两种方案都常见：调用语义用函数重载，类型计算用类模板偏特化。

不要把“显式特化函数模板”当成重载选择工具。重载决议先选主模板或普通函数，再在选中的是某个模板时考虑显式特化。若普通重载已经胜出，函数模板显式特化不会突然插队。实践里，函数定制优先写重载或 CPO；类型映射再用类模板偏特化。

## SFINAE 的边界

SFINAE 是 substitution failure is not an error。它只覆盖模板参数替换时、立即上下文里的失败。例如返回类型、形参类型、默认模板实参和 requires 表达式里的依赖表达式可以让某个模板从候选集中移除。

```cpp
template<class T>
auto size_of(T& x) -> decltype(x.size());
```

若 `T` 没有 `size()`，`decltype(x.size())` 在替换时失败，这个模板不可行，编译器可以继续看别的候选。

函数体里的错误不是 SFINAE：

```cpp
template<class T>
auto broken(T& x) {
    return x.size();
}
```

这个模板可能先成为最佳候选。实例化函数体时才发现 `x.size()` 不存在，那就是 hard error。另一个常见硬错误来自“替换时触发了别的模板实例化，而错误发生在那个模板体内部”。SFINAE 不是把所有模板错误都变软，它只管立即上下文。

`void_t` 检测习语把表达式放进立即上下文：

```cpp
template<class, class = void>
struct has_size : std::false_type {};

template<class T>
struct has_size<T, std::void_t<decltype(std::declval<T&>().size())>>
    : std::true_type {};
```

C++20 以后，很多这种写法可以改成 concept：

```cpp
template<class T>
concept has_size = requires(T& x) {
    x.size();
};
```

概念不是另一套魔法。它只是把“这个表达式是否良构”放在更直接的位置，并让诊断和重载排序有机会更清楚。L05 的 checker 会专门验证 no-path 类型不满足 `c04_describable`，说明错误停在候选集阶段。

## overload 与 specialization 的选择顺序

如果你写了函数模板主模板、函数模板显式特化和普通非模板重载，调用选择不是“最像的那段代码”。顺序是：先做名字查找，形成 overload set；对主模板推导并替换；做重载决议；若选中的是某个函数模板，再找它对应的显式特化。

因此：

```cpp
template<class T>
void draw(T);

template<>
void draw<int*>(int*);

void draw(int*);
```

调用 `draw(p)` 时，普通 `void draw(int*)` 可能直接在重载决议里胜出。函数模板显式特化不是新的 overload，它依附于主模板。很多定制点不建议依赖函数模板显式特化，就是因为它不按读者直觉参与候选排序。

## 练习目标

L05 要实现 `c04_overload::describe(object)`。它返回小的类型标签，checker 用类型和值一起验证：

1. 若对象有零参数 `describe()` 成员，返回 `member_result`，并实际调用该成员。
2. 若输入是字符串字面量 `const char(&)[N]`，返回 `literal_result<N>`，不能退化成 `const char*`。
3. 若输入是整数但不是 `bool`，返回 `integral_result`。
4. 若输入满足 range 且前面几类不成立，返回 `range_result`。
5. 无合法路径时不可调用。

这个题不要求写一个通用序列化库。它只让你看到：候选先被约束筛掉；数组引用比指针更能表达契约；函数模板重载替代函数模板偏特化；`requires` 比把错误留在函数体里更容易形成可诊断接口。

配套诊断 case 分成两组。“尝试偏特化函数模板”的 subject 证明函数模板偏特化非法，control 使用函数模板重载完成同样意图。“函数体 hard error”的 subject 把 `x.size()` 放进已选中模板的函数体并实际实例化，control 把同一个检查放到 trailing return type 的立即上下文并落到 fallback。两组 case 都不参与正常运行路径，只用于观察错误阶段。
