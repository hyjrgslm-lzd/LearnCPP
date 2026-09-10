# 14. Splicing与生成：从元信息回到代码

反射查询只回答“有什么”。要把答案用于程序，还要把`std::meta::info`放回语法位置。这个动作叫splicing，语法是`[: r :]`。P2996R13把splicing放进表达式、类型、命名空间等语法环境；哪个环境允许什么，取决于`r`表示的实体。

最小例子：

```cpp
struct Point {
    int x;
    int y;
};

void bump_x(Point& p) {
    constexpr std::meta::info x = ^^Point::x;
    p.[:x:] += 1;
}
```

`x`不是成员指针。`p.[:x:]`也不是运行时字符串查找。编译器在语法层面把成员元信息拼回成员访问表达式，因此结果保留成员类型、引用和值类别。字段格式化、tuple投影和序列化都依赖这个性质。

## 展开语句让每个成员都有自己的语法点

普通`for`循环只有一个运行时循环体，不能在每次迭代里改变表达式类型。字段`id`是`int`、字段`name`是`std::string`，同一个运行时变量无法同时承载两种成员表达式。C++26的[P1306R5](https://wg21.link/p1306r5)引入`template for`展开语句：编译器先求出展开大小，再为每个元素生成一份语句体。

```cpp
template<class T>
void print(T const& object) {
    template for (constexpr auto m :
        std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()))) {
        std::println("{}={}", std::meta::identifier_of(m), object.[:m:]);
    }
}
```

`template for`不是运行时循环。`auto elem`用于同构值时像普通局部变量；`constexpr auto elem`才让每个展开元素成为常量表达式，能参与`static_assert`、类型splicing或反射splicing。展开源可以是表达式列表、编译期大小range、tuple-like对象；直接“遍历类型”不是语法目标，类型域通常经`^^T`转成值域元信息再`using T = [:r:]`拼回类型。

`break`和`continue`控制的是执行流，不是实例化开关。P1306R5给出的语义是：`break`跳到最后一个展开体之后，`continue`跳到下一个展开体开始；但语句体已经按展开大小产生多个语法实例。因此下面这种“靠`break`躲开坏类型”的写法仍不可靠，后续展开体里的类型约束、名称查找或splicing格式错误仍可能在实例化/语义检查时暴露：

```cpp
template for (constexpr auto r : members) {
    if (should_stop(r)) {
        break;              // 停止执行后续体，不承诺停止后续体的语义形成
    }
    use_member(object.[:r:]); // 这里仍要对对应展开实例语义成立
}
```

如果某个成员不满足约束，用`if constexpr`或约束把坏路径排除；不要把`break`/`continue`当成SFINAE。

## `define_static_*`解决非瞬态存储

`nonstatic_data_members_of`返回`vector<info>`。这个`vector`属于常量求值临时分配；你不能把它当成普通全局数组留到运行时，也不能随便把它交给需要静态存储地址的语法。P3491R3把`std::define_static_string`、`std::define_static_array`、`std::define_static_object`放在`namespace std`，不是`std::meta`。

关键点：

- `std::define_static_array(r)`返回`std::span<const std::ranges::range_value_t<R>>`；
- 它把输入范围的值物化到静态存储，使展开语句可以稳定遍历；
- 对`vector<meta::info>`这类常量求值结果，不能先当运行时vector保存，再晚点遍历；
- 字符串生成用`define_static_string`，对象生成用`define_static_object`，不要用手写全局缓存伪装。

课程代码中出现`std::define_static_array(std::meta::nonstatic_data_members_of(...))`时，目的是把“查询得到的瞬态范围”转换成“展开语句可用的静态范围”。

## `define_aggregate`生成类型定义

`data_member_spec`和`define_aggregate`负责另一类工作：不是访问已有字段，而是按元信息生成类定义。P2996R13示例中，先声明不完整类型，再在`consteval`上下文里用`define_aggregate(^^T, specs)`补出成员。

```cpp
template<class T>
struct storage_for;

consteval {
    std::meta::define_aggregate(^^storage_for<int>, {
        std::meta::data_member_spec(^^int, {.name = "value"})
    });
}
```

这类生成适合“从schema生成聚合”或“把struct of arrays/schema转成类型”。C++23主线字段codec使用手工member pointer描述器；P1另有真实反射后端 `src/reflection/record_ops.hpp`，本机因缺少对应能力而未运行。两种路径比较同一支持域，不能把手工路径通过算成反射通过。考虑如何设计反射接口时，下面两个声明可以作为待分析的候选：

```cpp
template<class T>
consteval auto reflected_fields(std::meta::access_context ctx)
    -> std::span<const std::meta::info>;

template<class T>
decltype(auto) reflected_member(T&& object, std::meta::info member);
```

真实实现里第二个接口不能写成普通运行时函数，因为`object.[:member:]`要求`member`在展开产生的语法点上是常量。更实际的公共形态是`visit_fields_reflection(T&&, F&&, access_context)`，内部用`template for`逐字段调用callback；C++23 backend则用member pointer tuple走相同callback契约。P1 checker只需要相同支持域和相同错误语义，不需要把反射元信息暴露成运行时容器。

## 格式化和序列化不要越界

反射让“拿字段名和值”变容易，但不自动定义格式协议。格式化可以跳过没有`operator<<`或`std::format`支持的字段；序列化必须稳定处理缺失、未知、重复、非法值、版本迁移和名称变更。annotations可以给格式化示例做`skip`，但实施规格已经限定codec全部字段往返。否则一个字段因为展示层标注被静默丢进持久化格式，会变成数据损坏。

本章练习里`splicing_generation_probe.cpp`、`expansion_statement_probe.cpp`、`define_static_probe.cpp`和`define_aggregate_probe.cpp`分别检查这四件事。拆成四个源文件，是为了避免“反射头不可用”遮住“pack indexing其实可用”之类错误归因。
