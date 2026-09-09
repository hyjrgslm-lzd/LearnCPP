# 04：`std::variant` 与有限状态集合

当一个对象在任一时刻只可能属于几种已知形态之一时，状态集合应该进入类型。`std::variant<A, B, C>` 表达“持有一个 `A`，或一个 `B`，或一个 `C`”。它比“一个 `struct` 加多个 `bool` 标记”更接近真实模型，因为非法组合无法表示。

图形文档是本课主案例。最终 `Shape` 使用：

```cpp
struct Rectangle { Length width; Length height; };
struct Circle { Length radius; };
using Shape = std::variant<Rectangle, Circle>;
```

这个签名说明形状集合是封闭的：当前课程只允许矩形和圆。新增 `Line` 不是插件配置，而是改类型定义、改所有访问分支、重新构建。封闭集合正是 `variant` 的优点：编译器可以把遗漏分支暴露出来。

## 状态空间先于代码

手写 tag 常见写法如下：

```cpp
enum class Kind { rectangle, circle };

struct Shape {
    Kind kind;
    Rectangle rectangle;
    Circle circle;
};
```

这个类型实际能表示的状态远大于业务状态。`kind == rectangle` 时 `circle` 仍存在；两个字段可能都含旧值；维护代码必须人工保证“只读当前 kind 对应字段”。`variant<Rectangle, Circle>` 把状态空间压回两种：当前替代项是 `Rectangle` 或 `Circle`，不存在“kind 是矩形但圆字段也被当成有效业务状态”的组合。

这也解释了 `optional`、`variant`、继承和类型擦除的分工：`optional<T>` 是 0/1 个 `T`；`variant<A,B>` 是固定备选集合；虚接口和类型擦除用于开放集合，但要额外处理对象所有权、复制、析构和 ABI 边界。

## 构造选择与重复类型

`variant` 的默认构造会默认构造第 0 个替代项。如果第 0 个类型不能默认构造，整个 `variant` 也不能默认构造，除非把 `std::monostate` 放在第 0 位表达显式空闲状态。

```cpp
using State = std::variant<std::monostate, Editing, Submitted>;
State s; // 当前是 monostate
```

从一个值构造 `variant` 时，标准会按重载规则选择一个可行且不歧义的替代项。若多个替代项都能从同一个实参构造，代码可能无法编译，或者选择的并不是你以为的业务状态。需要精确选择时，用 `std::in_place_type<T>` 或 `std::in_place_index<I>`：

```cpp
std::variant<int, std::string> v{std::in_place_type<std::string>, "42"};
```

重复类型是合法的，但很多按类型访问的接口会失效。`std::variant<int, int>` 有两个 `int` 替代项；`std::get<int>(v)` 和 `std::holds_alternative<int>(v)` 因为类型不唯一而不能使用。此时只能用索引访问：

```cpp
std::variant<int, int> two{std::in_place_index<1>, 7};
auto second = std::get<1>(two);
```

业务模型里重复类型通常是坏味道，因为两个 `int` 的含义不同但类型相同。更清楚的做法是强类型包装：

```cpp
struct Width { int value; };
struct Height { int value; };
using Dimension = std::variant<Width, Height>;
```

## 访问：`get`、`get_if`、`holds_alternative`

`std::holds_alternative<T>(v)` 检查当前是否为唯一的 `T` 替代项。`std::get<T>(v)` 或 `std::get<I>(v)` 在状态不匹配时抛 `std::bad_variant_access`。`std::get_if<T>(&v)` 或 `std::get_if<I>(&v)` 返回指针，状态不匹配时返回 `nullptr`。

```cpp
if (const auto* circle = std::get_if<Circle>(&shape)) {
    return area(*circle);
}
```

选择接口时看失败是不是正常分支。若“不是圆”只是状态机普通分支，`get_if` 更直接；若上层已证明当前必为圆，`get` 可以作为契约检查，失败就是上层逻辑错。不要先 `holds_alternative` 再散落多个 `get`，那会把状态分发拆碎；公共操作通常写成 `visit`。

## `visit`：穷尽、返回类型和多 variant

`std::visit(visitor, v)` 会根据当前替代项调用对应重载。访问器必须能处理 `variant` 可能持有的每一种替代项。常见写法是用重载 lambda：

```cpp
template<class... Fs>
struct overload : Fs... { using Fs::operator()...; };
template<class... Fs>
overload(Fs...) -> overload<Fs...>;

double area(const Shape& shape) {
    return std::visit(overload{
        [](const Rectangle& r) { return r.width.value * r.height.value; },
        [](const Circle& c) { return pi * c.radius.value * c.radius.value; }
    }, shape);
}
```

访问器的所有分支要形成一个统一返回类型。返回 `int` 和 `double` 通常会被共同类型规则处理，但返回 `std::string` 和 `double` 没有共同业务含义，应该显式改成同一类型，或把操作拆开。`visit<R>` 可以指定返回类型，但它不是掩盖设计混乱的工具。

多 `variant` 访问会对当前替代项组合做分发：

```cpp
std::visit([](const auto& lhs, const auto& rhs) {
    return intersects(lhs, rhs);
}, a, b);
```

若第一个有 3 种、第二个有 4 种，访问器需要覆盖 12 种组合。组合数量上升很快。有限状态集合不等于任意堆叠状态集合都好维护；当组合规则有结构时，可以先归一化、拆成小操作，或重新建模。

## 赋值、`emplace` 与 `valueless_by_exception`

`variant` 正常情况下 never-empty：它持有一个替代项。但这不是“所有操作都不抛异常”。替代项构造、移动、拷贝、赋值都可能抛。若改变替代项时旧对象已经销毁、新对象构造又失败，`variant` 可以进入 `valueless_by_exception()` 状态。

```cpp
std::variant<int, Throwing> v = 1;
try {
    v.emplace<Throwing>(); // Throwing 构造抛出
} catch (...) {
    // v 可能 valueless_by_exception
}
```

标准允许实现用不同策略降低这个风险，例如先构造临时对象再提交；不同操作、不同替代项的 noexcept 属性也会影响路径。因此课程结论要分清：规范允许 valueless；某次本机观察只说明当前实现和当前类型组合的行为。接口不能依赖“我机器上没见过 valueless”。

若业务不允许中途没有有效状态，应把可能抛的构造放在外部 prepare 阶段，成功后再提交，或选择替代项操作都不抛的类型。这个思路会在第 06 章异常安全中展开。

## `monostate`、移动和复制

`std::monostate` 是一个空替代项，常用来表达“尚未选择任何业务状态”或让 `variant` 可默认构造。它不是错误通道，也不是 `optional` 的替代品。若状态只有“无/有一个 T”，`optional<T>` 更直接；若无状态只是多状态机中的一个显式阶段，`variant<monostate, A, B>` 合理。

`variant` 的复制、移动能力由所有替代项共同决定。只要某个替代项不可复制，整个 `variant` 的复制构造就不可用；移动同理。移动一个 `variant` 会移动当前替代项，源对象仍是合法 `variant`，但其中替代项的值遵守该类型的 moved-from 规则。不要把“variant 被移动”误解成它自动变空。

## C++26 成员 `visit`

C++26 增加 `variant` 成员 `visit`，调用形式从 `std::visit(visitor, v)` 变成 `v.visit(visitor)`。它改善可读性和链式组织，不改变状态空间、穷尽检查、返回类型合并或异常规则。本机是否支持由 `../exercises/F01_frontier/capabilities/c26_variant_member_visit.cpp` 探测；当前课程主线使用 C++23 的非成员 `std::visit`。

## L04 练习

`L04_variant` 是观察型练习，使用有限图形和订单状态机验证：

- `variant` 默认构造和 `monostate` 的含义。
- `get`、`get_if`、`holds_alternative` 的前提与失败路径。
- 重复类型必须按索引访问，业务上优先强类型。
- `visit` 对单个和多个 `variant` 的穷尽分发。
- `emplace` 构造失败可能留下 `valueless_by_exception`，本机观察只绑定当前实现。
- 移动后源对象仍合法，但不承诺业务值不变。

完整解析：`variant` 的价值不是“比虚函数快”，本课不做性能排名。它的价值是把封闭状态集合写进类型，让非法组合不可表示，并让新增替代项时缺失的公共操作在编译期暴露。开放插件、跨 ABI 扩展和运行期未知类型在第 09、11、13、15 章另讲。
