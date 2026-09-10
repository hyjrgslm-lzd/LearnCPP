# 13. 反射模型：`std::meta::info`不是“类型对象”

本章进入C++26静态反射。前面章节靠模板参数、traits、tuple和member pointer在语言边缘绕出“字段表”。这些技术能完成C++23项目，但它们没有真正询问语言实体：字段名从字符串维护，访问控制靠约定，枚举名靠手写表，私有成员要么不可见，要么被非标准技巧绕过。静态反射的目标是让程序在常量求值中得到“某个声明、类型、命名空间、成员、枚举项”的元信息，然后用标准查询回答问题。

规范基线固定为[P2996R13](https://wg21.link/p2996r13)和C++26草案[N5050](https://wg21.link/n5050)。实现状态单独看，不用本机MSVC没有`<meta>`推出语言本身不存在。

## 元信息值表示实体

`^^T`、`^^object`、`^^namespace_name`这类反射操作符产生`std::meta::info`值。这个值不是`T`的运行时对象，也不是`type_info`。它是常量求值中可比较、可传递、可查询的不透明元信息值。`^^int`表示类型实体`int`；`^^Widget::id`表示某个成员；`^^Color::red`表示枚举项。它们同属`std::meta::info`，但后续查询有前提。

最常见错误是把“实体”和“对象”混在一起：

```cpp
struct Record {
    int id;
};

constexpr std::meta::info type_meta = ^^Record;     // Record这个类型
constexpr std::meta::info field_meta = ^^Record::id; // id这个非静态数据成员
```

`type_meta`可以传给`nonstatic_data_members_of`。`field_meta`不能当作类型去枚举字段，但它可以问名称、声明类型、访问性，之后也可以被splicing用于成员访问。反射API通常不会把错误输入解释成空结果；很多查询要求前提满足，前提不满足就是编译期诊断或常量求值失败。课程代码因此先写`has_identifier`、`is_type`、`is_nonstatic_data_member`之类的检查，再调用`identifier_of`、`type_of`或成员访问。

`identifier_of(r)`要求`r`有语言标识符。匿名字段、构造函数、某些运算符或实现产生的实体不一定满足。教学代码不能把`display_string_of`当成稳定schema名；它适合诊断，不适合序列化格式。`type_of(r)`也不是“随便给一个info就返回类型”：对数据成员它问成员声明类型，对枚举项它问枚举类型；对类型反射本身，需要用splicing或特定查询得到语言类型。

## 查询返回的是编译期范围

对类类型，核心入口是：

```cpp
using namespace std::meta;

consteval bool has_two_public_fields(info type) {
    auto fields = nonstatic_data_members_of(type, access_context::current());
    return fields.size() == 2;
}
```

`nonstatic_data_members_of(info, access_context)`返回`vector<info>`，顺序按声明顺序。这个顺序能支撑字段格式化、tuple投影和聚合生成。但返回的`vector<info>`活在常量求值里，不能按普通运行时容器思考。要把它交给`template for`或跨越非瞬态存储边界，需要第14章的`std::define_static_array`一类工具。

对枚举，`enumerators_of(^^Color)`给枚举项元信息。枚举名到字符串的练习只接受有identifier的枚举项；没有名称的值不是枚举项实体，不能靠遍历发现。

字段遍历的最小真实形态是：

```cpp
template<class T>
void print_fields(T const& value) {
    template for (constexpr auto m :
        std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()))) {
        std::println(".{}={}", std::meta::identifier_of(m), value.[:m:]);
    }
}
```

这里没有手写traits。`m`是成员元信息，`value.[:m:]`才是运行时对象的成员访问。元信息决定“访问哪个成员”，对象提供“从哪个对象取值”。

## `access_context`是语义的一部分

反射不是关闭C++访问控制的调试开关。P2996R13给`std::meta::access_context`定义了`current()`、`unprivileged()`和`unchecked()`三种核心语义。

`current()`按当前求值点和友元关系判断访问。例如类的friend consteval函数里拿到的context可以访问私有成员；普通外部代码拿不到。`unprivileged()`表示没有特权的外部视角，适合公开API和序列化边界。`unchecked()`跳过访问检查，适合编译器工具、教学观察或明确不用于公共库契约的实验。它不应成为业务字段codec的默认值。

所以“字段序列化”项目默认只处理公开普通成员聚合。若要支持私有字段，必须改变产品契约：是用friend提供context，还是由类型显式授权，还是只做诊断工具。这不是一个隐藏参数能解决的问题。

## 字段和枚举的边界

字段反射能枚举非静态数据成员，但不是所有成员都应该格式化或序列化。位域、匿名union、静态成员、成员函数、继承成员、私有成员、引用成员、不可格式化类型、不可稳定命名实体，都需要单独规则。课程综合项目先选“公开普通聚合的`int`、`bool`、`std::string`字段”，因为这个支持域能在C++23手工描述器和C++26反射backend之间比较。

枚举反射也有边界：遍历枚举项能实现`Color::red -> "red"`，但任意运行时整数转成`Color`后不一定对应某个枚举项。反射不会自动替你定义非法值策略。

## 本章练习

F01的`reflection_query_probe.cpp`是观察实验，不是学生实现。它检查：

- 是否能包含`<meta>`；
- 是否有明确反射能力宏或显式实验宏；
- 是否能用`^^`得到类型、字段和枚举项；
- 是否能查询字段顺序、名称、声明类型和访问上下文；
- 是否能在`consteval`局部使用查询结果，并只返回普通布尔/整数结论。

没有能力时返回77并记录SKIP。若工具链宣称能力但真实源码编译或运行失败，就是FAIL。这个规则故意严格，因为“浏览器支持表说支持”和“本课这段源码在本机通过”不是同一件事。
