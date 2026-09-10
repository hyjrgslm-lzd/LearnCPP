# 19. 字段选择 DSL：从名称到类型安全投影

先修：[静态字段工具](16-static-record.md)给出了公开普通成员聚合的手工 schema：字段名稳定、字段顺序稳定，支持 int、bool、string。这里继续沿用同一类对象，但目标换成“按名字取字段”和“按名字列表生成投影”。

最小正确基线仍然是手写访问：

```cpp
int& id(Person& p) { return p.id; }
auto pick_id_name(Person& p) { return std::tie(p.id, p.name); }
```

这段代码没有泛型噪声，也保留了引用。它的问题只在扩展：当调用点想写 `get<"id">(p)` 或 `project<"id,name">(p)` 时，字段名需要在编译期解析，未知字段、重复字段和拼错的列表应当在编译期拒绝，而不是运行到某个默认分支。

## 结构化字符串字面量

`get<"id">(p)` 能写成这样，是因为 `"id"` 先构造成一个结构化 NTTP：

```cpp
template<std::size_t N>
struct fixed_string {
    char value[N]{};
    constexpr fixed_string(const char (&text)[N]) {
        for (std::size_t i = 0; i < N; ++i) value[i] = text[i];
    }
    constexpr std::size_t size() const { return N - 1; }
};
```

`fixed_string<3>{"id"}` 的数组内容是 `{ 'i', 'd', '\0' }`，但 `size()` 是2。编译期解析时必须用长度，而不能把它当C字符串；否则嵌入NUL的 `"id\0name"` 会被误读成 `"id"`。

schema 的字段描述器也使用同一类 key：

```cpp
template<fixed_string Name, auto Member>
struct field {
    static constexpr auto name = Name;
    static constexpr auto member = Member;
};

template<>
struct schema<Person> {
    static constexpr auto fields = std::tuple{
        field<"id", &Person::id>{},
        field<"active", &Person::active>{},
        field<"name", &Person::name>{}
    };
};
```

字段名和成员指针都进入类型/常量表达式世界。运行时对象只在最后一步出现：选择好的成员指针作用到调用者传进来的对象上。

## 名字选择不是字符串查表

实现把 `"id"` 与 schema 中的字段名在编译期比较，找到后生成真实成员表达式：

```cpp
std::forward<T>(object).*descriptor.member
```

因此它保留 cv/ref。`Person&` 得到 `int&`，`const Person&` 得到 `const int&`，`Person&&` 得到可移动的成员表达式。这个接口不分配、不构造字段表，也不会把成员复制成临时值。

未知字段属于接口错误。与运行时 map 不同，这里没有“查不到返回 null”的默认语义；拼错的字段名会让模板实例化失败。错误越早，调用点越接近，后续基于字段类型的表达式也越安全。

如果写成运行时字符串查表，返回类型只能统一成 variant、指针或擦除后的包装；那会丢掉 `int&`、`std::string&` 这些表达式类型。这里的模板 key 让返回类型在编译期已经确定。

## 投影先解析，再形成表达式类型

`project<"id,name">(p)` 把逗号分隔的字段名解析成一个类型层面的列表，再按列表顺序调用 `get`，最后返回 `std::tuple` 引用。投影顺序来自调用者写下的 DSL，而不是 schema 顺序。

允许的语法刻意很小：空串表示空投影；字段名用 ASCII identifier，首字符是字母或下划线，后续可以含数字；逗号之间不能有空项。不支持空白、转义、引号和嵌套语法。需要更复杂的查询语言时，应进入运行时 parser 或专门 DSL，不把教学样例膨胀成半个 SQL。

重复字段也拒绝。`project<"id,id">(p)` 看似可以返回两个引用，但这会让写回、别名和后续 meta map 教学混在一起。这里的契约是“选择一组唯一字段并保留顺序”。

手推 `project<"name,id">(person)`：

1. 输入 `fixed_string<8>{"name,id"}`，有效字符长度为7。
2. 从位置0找到逗号位置4，切片得到 `fixed_string<5>{"name"}`。
3. 从位置5继续到末尾，切片得到 `fixed_string<3>{"id"}`。
4. 解析结果的类型是 `name_list<"name", "id">`。这一步的值不是运行时 vector，而是一组模板实参。
5. 对 `Person` 查 schema，`"name"` 对应 `&Person::name`，`"id"` 对应 `&Person::id`。
6. 对 `Person&` 形成两个表达式：`person.name` 是 `std::string&`，`person.id` 是 `int&`。
7. 返回类型因此是 `std::tuple<std::string&, int&>`，顺序与 DSL 一致。

这也是为什么普通 `consteval` 局部变量不能直接接上模板调用。下面的写法表达了一个常见但错误的直觉：

```cpp
#include <array>
consteval auto invalid_extent(int count) {
    return std::array<int, count>{}; // 反例：普通参数 count 不是模板实参所需常量表达式
}
```

模板实参必须在实例化点形成。合法常量表达式初始化的 `constexpr` 局部变量可以作模板实参；普通参数和非 `constexpr` 局部不会因外层是 `consteval` 自动具备这个性质，[第10章](10-constant-evaluation.md)有独立诊断正反例。本题可以让解析形成 `name_list<"name", "id">`，再展开 `get<Names>(object)...`；也可以先形成 `constexpr` 索引数组，再按 `index_sequence` 生成成员表达式。Reference 选择前者，把错误定位到对应切片和字段。两种组织都必须保持同一个结果类型、顺序和借用契约；helper 命名不同本身不是独立正确性的证据。

先复现问题，再修：观察程序先展示“复制字段”和“按 schema 顺序返回”都能读到值但不是投影；checker 随后要求 `project<"name,id">` 写回原对象。bad 变体可编译，但会在这条写回检查上失败。

## 借用边界

`project` 只接受左值，包括 const 左值：

```cpp
Person p{7, true, "Ada"};
auto [id, name] = project<"id,name">(p);        // id 是 int&，name 是 string&
auto [cid] = project<"id">(std::as_const(p));  // cid 是 const int&
```

它不接受临时对象。tuple 中存的是成员引用，如果允许 `project<"name">(Person{})`，引用会在完整表达式结束后悬垂。`get` 仍可用于右值，因为单个表达式可以直接消费成员；但调用者若把 `get<"name">(Person{})` 的引用保存起来，同样会悬垂。`project` 的限制是一条 API 策略：聚合多个借用时默认只允许已有对象。

## 约束与错误反例

最常见的错误实现有两类。第一类把字段复制出来，检查时能读到值，却不能写回原对象，也丢失 const/ref 信息。第二类先按 schema 顺序拼 tuple，忽略 DSL 中的顺序；`project<"name,id">` 会返回错误类型和值序。

[A02练习](../exercises/A02_field_projection/README.md)先跑固定字段基线和反例，再实现完整投影。检查器覆盖名称解析、投影顺序、写回、const引用、空投影、未知字段、重复字段、非法 identifier、空组件和拒绝临时对象。后续 Mp11 样章会复用同一组记录与 key 形态，把手写解析迁移到成熟元编程库。

自测：

1. 为什么 `get` 可以转发右值，而 `project` 只收左值？前者返回一个可立即消费的成员表达式；后者返回保存引用的 tuple，临时对象会在完整表达式结束后销毁。
2. 为什么 `project<"id,name">` 不按 schema 顺序返回？调用者显式写了 DSL，投影的可读含义就是这个顺序。
3. 为什么拒绝重复字段？本章的契约是唯一字段集合；重复引用会引入别名写回语义，留给后续更明确的接口。
4. 为什么不支持空白？小 DSL 的边界越清晰，编译期诊断越稳定；需要用户输入友好性时应使用运行时 parser。
