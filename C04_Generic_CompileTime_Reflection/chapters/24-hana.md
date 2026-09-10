# 24. Boost.Hana：编译期值、异构 map 与运行时对象

先修：[字段选择 DSL](19-field-projection-dsl.md)、[Boost.Mp11](23-mp11.md)。Mp11 主要处理“类型列表”；Hana 更强调“编译期值”和“异构对象”的统一写法。

最容易误解的一点：Hana 让编译期值看起来像对象，但它没有让运行时值决定新 C++ 类型。类型仍在编译期确定；运行时对象只是被这些编译期 key 和类型引导访问。

## type_c 与 integral_c

`hana::type_c<T>` 是一个代表类型 `T` 的值。它能放进 map，也能参与比较：

```cpp
constexpr auto t = boost::hana::type_c<int>;
static_assert(t == boost::hana::type_c<int>);
```

`hana::integral_c<T, v>` 是代表整数常量的值：

```cpp
constexpr auto n = boost::hana::integral_c<std::size_t, 3>;
static_assert(n == boost::hana::size_c<3>);
```

这两个对象都没有把运行时数据提升成类型。它们只是让类型层信息以值语法参与组合。

## 编译期字符串 key

Hana 的字符串 key 常用 `BOOST_HANA_STRING("id")`：

```cpp
constexpr auto id = BOOST_HANA_STRING("id");
constexpr auto schema = boost::hana::make_map(
    boost::hana::make_pair(id, boost::hana::type_c<int>)
);
```

`hana::contains(schema, id)` 和 `hana::at_key(schema, id)` 都在编译期选择。key 拼错时，`at_key` 是接口错误；需要可选查询时用 `hana::find`，它返回 optional-like 结果。

## 类型 schema 与运行时值分开

字段工具里通常有两张表：

```cpp
constexpr auto type_schema = hana::make_map(
    hana::make_pair(BOOST_HANA_STRING("id"), hana::type_c<int>),
    hana::make_pair(BOOST_HANA_STRING("name"), hana::type_c<std::string>)
);

Person person{7, true, "Ada"};
auto values = hana::make_map(
    hana::make_pair(BOOST_HANA_STRING("id"), &person.id),
    hana::make_pair(BOOST_HANA_STRING("name"), &person.name)
);
```

第一张表回答“字段是什么类型”。第二张表保存“这个对象的字段在哪里”。`values` 可以借用运行时对象，因此它必须短于 `person` 的生命周期。不要把 `values` 当成拥有对象，也不要把用户输入字符串直接当成 Hana key 生成新类型。

如果 value 保存的是副本，修改 map 不会修改原对象：

```cpp
auto copied = hana::make_map(
    hana::make_pair(BOOST_HANA_STRING("id"), person.id)
);
hana::at_key(copied, BOOST_HANA_STRING("id")) = 42;
// person.id 没有变。
```

如果 value 保存的是 `std::ref`，修改 `get()` 才会写回原对象：

```cpp
auto borrowed = hana::make_map(
    hana::make_pair(BOOST_HANA_STRING("id"), std::ref(person.id))
);
hana::at_key(borrowed, BOOST_HANA_STRING("id")).get() = 42;
// person.id 变成 42。
```

这里的 const 也要按 C++ 对象模型理解。`const auto map = borrowed;` 只让 Hana map 这个对象不能被重新赋值；map 中的 `std::reference_wrapper<int>` 仍然可以通过 `get() const` 返回 `int&`。所以 const map 不会自动把被引用的 `person.id` 变成 const。想表达只读借用，应保存 `std::reference_wrapper<const T>` 或指向 const 的指针。

迁移一个陌生 Record 时，可以按四步做：

1. 为每个字段写固定的编译期 key，例如 `BOOST_HANA_STRING("voltage")`。
2. 建 `type_schema`，把 key 映射到 `hana::type_c<T>`，用 `static_assert` 证明字段类型和字段数。
3. 建运行期 value map。需要只读快照就存副本；需要写回对象就存指针或 `std::ref`，并保证被借用对象活得更久。
4. 缺键路径用 `hana::find`；必须存在的 key 才用 `hana::at_key`。`at_key` 的失败应被视为接口写错，不是普通运行时分支。

## 和 Mp11 的取舍

Mp11 写法更接近 type trait，适合纯类型计算、完成签名、schema 归一化和大批量类型变换。Hana 写法更接近普通表达式，适合演示编译期值、异构记录和 key-value 组合。

MPL 是历史背景：它展示了早期模板元编程如何在 C++11 前后组织算法，但本课新代码不从 MPL 开始。现代教学先用手写最小模型理解机制，再用 Mp11/Hana 看成熟库如何压缩样板。

## 迁移练习

[U02观察](../exercises/U02_hana/README.md)先复用 A02 的 `Person`：一边用 `hana::type_c` 表示字段类型，一边用 `hana::map` 保存运行时成员指针。随后 `U02_hana_migration_solution` 换成 `SensorReading`，要求你解释编译期 key map 迁移、运行期值更新、copy 与 `std::ref` 借用、缺键 `find`、`at_key` 前置条件、类型证明和 const map 的借用边界。

迁移问题：

1. 如果字段 key 来自用户输入，为什么不能直接生成 `BOOST_HANA_STRING(user_text)`？宏参数必须是编译期字面量；用户输入属于运行时数据。
2. `hana::find` 与 `hana::at_key` 的接口区别是什么？前者表达可选查询，后者表达必须存在。
3. 为什么 runtime value map 存指针或引用时要关心生命周期？Hana map 不会替你拥有 `person`，它只保存异构值。
