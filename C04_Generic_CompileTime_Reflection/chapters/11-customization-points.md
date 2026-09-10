# 11. 自定义点：从已知类型函数到 `read_value` CPO

很多模板教材会先讲一串术语：`decltype(auto)`、转发引用、ADL、Concepts、`noexcept`、hidden friend，然后给一个很短的 `tag_invoke` 或 CPO 例子。这样读者容易记住写法，却不知道每一层到底在防什么错。本章反过来，从一个普通业务函数开始。只有当需求真的变化、原来的代码真的破坏契约时，才引入下一层泛型机制。

本章要实现的接口叫 `c04::read_value(x)`。它不是为了展示花哨技巧，而是一个小而完整的 customization point object：如果对象自己有可调用的 `read_value()` 成员，就优先调用成员；否则尝试通过 ADL 找到与对象类型同 namespace 的自由函数 `read_value(x)`；两条路都不存在时，`c04::read_value(x)` 不能被调用。正确实现还要保留返回值的真实类型和值类别，不能把引用拷成值，不能把 `const` 或右值情况误转成普通左值，不能谎报 `noexcept`。

读本章只需要 C01 的构建/诊断基础和 C02 的值类别、移动、引用、对象生命周期基础。模板深水区的词会在用到前局部解释，不要求先读完本课程后续章节。

## 已知类型的正确基线

先看一个没有模板的版本：

```cpp
struct Sensor {
    int value{};
    int& read_value() & noexcept { return value; }
    const int& read_value() const& noexcept { return value; }
};

int& read_known_sensor(Sensor& sensor) noexcept {
    return sensor.read_value();
}
```

这段代码已经有几个正确契约。

第一，它返回 `int&`，不是 `int`。调用者拿到的是对象内部那个 `value`，所以可以检查地址、可以修改原对象。若写成 `int read_known_sensor(Sensor&)`，表面结果相同，引用身份已经丢了。很多泛型 helper 的第一个 bug 就是这里：单元测试只比对数值，没比对是不是同一个对象。

第二，参数是 `Sensor&`，所以它只接受非常明确的输入。`const Sensor&`、临时对象、别的 namespace 里的类型都不参与。这不是缺陷，只是需求还没变。

第三，`noexcept` 与真实调用一致。因为成员函数是 `noexcept`，wrapper 也能承诺不抛。若成员以后变成可能抛，外层还写死 `noexcept`，异常穿过时会直接 `std::terminate()`。

所以泛型化前的基线不是“能打印一个值”，而是这三件事：调用正确对象、保留返回身份、异常承诺真实。

练习里的 `observations/baseline.cpp` 只证明这个正确基线：已知类型 wrapper 能保持引用身份，写回能落到原对象，`const` wrapper 不偷改对象。它不是泛型答案，只是后面每次扩展都不能破坏的起点。

## 第一次泛型化：不要先丢契约

需求变化：现在有多个类型都提供成员 `read_value()`。最直接的模板是：

```cpp
template<class T>
decltype(auto) read_member_value(T&& object)
    noexcept(noexcept(std::forward<T>(object).read_value()))
{
    return std::forward<T>(object).read_value();
}
```

这里出现三个新点。

`T&&` 在函数模板中，且 `T` 需要从实参推导，这种参数常叫转发引用。传入左值时，`T` 会推导成左值引用；传入右值时，`T` 通常推导成非引用类型。它的目的不是“万能接受”，而是把调用者给你的值类别保存下来。

`std::forward<T>(object)` 是保存值类别的动作。若调用者给的是左值，它仍是左值；若调用者给的是右值，它才恢复成右值。没有这一句，函数体里的具名变量 `object` 永远是左值，右值限定成员函数就匹配不到，或者错误调用到左值重载。

`decltype(auto)` 是保存返回类型和值类别的动作。`auto` 会丢掉顶层引用，`int&` 会变成 `int`；`decltype(auto)` 对 `return expression;` 保留表达式真实类型。对 CPO 这类薄 wrapper，这通常是正确选择：wrapper 不应该改变被包装调用的返回语义。

最后的 `noexcept(noexcept(expr))` 不是重复写错。内层 `noexcept(expr)` 是一个编译期布尔值，询问表达式会不会抛；外层 `noexcept(...)` 把这个布尔值变成函数自己的异常规格。也就是说，成员不抛时 wrapper 不抛，成员可能抛时 wrapper 也不承诺。

到这里，我们只解决“多个成员类型”问题，还没有解决非侵入扩展。

这一段的错误版本也有可运行观察，放在 `observations/counterexamples.cpp`。它先构造一个 `auto` 返回的 wrapper，成员实际返回 `int&`，但 wrapper 返回值只是拷贝；修改返回值不会改原对象，类型检查也显示它不是 `int&`。然后构造一个忘记 `std::forward` 的 wrapper：直接对右值对象调用成员会选中 `&&` 限定重载，而通过具名参数 `object.read_value()` 会把对象当左值，选中 `&` 限定重载。这两个反例不依赖最终 Reference 实现，因此能证明“为什么需要 `decltype(auto)` 和 `std::forward`”，而不是只证明最终答案碰巧通过。

同一个 observation 还检查写死 `noexcept` 的问题。它不实际调用会抛异常的 `noexcept` wrapper，因为那会触发 `std::terminate()`，不适合作为普通教学测试；它只用 `noexcept(expr)` 比较 wrapper 的承诺和底层成员的真实异常规格。这个观察的边界很明确：它证明异常规格被谎报，不演示进程终止路径。

## 为什么需要 ADL 路径

有些类型不能或不应该改成员。例如第三方类型、标准库类型、跨模块边界类型，或者你希望把读取策略放在类型旁边但不进类定义。于是会写：

```cpp
namespace app_model {
struct Field {
    int value{};
};

int& read_value(Field& field) noexcept {
    return field.value;
}
}
```

调用方若写 `read_value(field)`，普通未限定函数调用会触发 ADL，也就是 argument-dependent lookup。编译器除了当前作用域可见的函数，还会去实参类型关联的 namespace 找同名函数。`app_model::Field` 的关联 namespace 是 `app_model`，所以 `app_model::read_value(Field&)` 能被找到。

ADL 的价值是扩展点可以放在类型旁边。它的风险是名字查找容易被污染，尤其当 CPO 自己也叫 `read_value` 时，如果实现里不隔离，可能递归调用自己。

错误写法类似这样：

```cpp
namespace c04 {
inline constexpr struct read_value_fn {
    template<class T>
    decltype(auto) operator()(T&& object) const {
        return read_value(std::forward<T>(object)); // 这里可能又找到 c04::read_value 这个对象
    }
} read_value{};
}
```

`read_value` 既是对象名，又想作为 ADL 函数名。若查找落回对象本身，就会递归或形成难懂的候选集错误。正确做法是把 ADL 调用放在内部 detail namespace，并放一个不可用的同名 poison pill：

```cpp
namespace c04::detail {
void read_value();

template<class T>
concept has_adl_read_value =
    requires(T&& object) {
        read_value(std::forward<T>(object));
    };
}
```

`void read_value();` 没有参数，本身不能匹配一元调用，但它让普通未限定查找停在 detail namespace，不再向外层找到 `c04::read_value` 对象。随后 ADL 仍会加入实参关联 namespace 里的 `read_value` 函数。这样既保留扩展，又避免 CPO 自调用。

`counterexamples.cpp` 对 ADL 污染也只做安全观察。它构造一个故意未隔离的 CPO，并给递归路径加深度哨兵，避免真正无限递归。测试证明一个没有成员、没有 ADL 自由函数的对象仍被错误接受，并且调用会回到 CPO 自己。这只能证明“未隔离查找会让无路径对象进入候选并产生递归风险”；它不把无限递归跑到栈溢出，因为那属于破坏性现象，不适合放进默认构建。

## 用 `requires` 把错误留在候选集阶段

现在我们要表达两条合法路径：

1. 若 `object.read_value()` 对当前 cv/ref 条件可调用，调用成员。
2. 否则，若未限定 `read_value(object)` 能经 ADL 找到合法函数，调用 ADL。
3. 两者都不成立时，`c04::read_value(object)` 不是可调用接口。

这不是运行时 `if`，而是候选函数约束。`requires` 表达式会检查表达式是否良构。若不良构，概念值为 `false`，不会把整次编译炸掉。

```cpp
template<class T>
concept member_readable =
    requires(T&& object) {
        std::forward<T>(object).read_value();
    };

template<class T>
concept adl_readable =
    requires(T&& object) {
        read_value(std::forward<T>(object));
    };
```

这里的 `T&&` 与真实 `operator()` 一致，所以它会区分 `T&`、`const T&`、`T&&`。一个成员只写了 `int read_value() &`，就不应该让 `const T&` 或右值通过。泛型检查若把对象写成 `T object` 或 `T& object`，会悄悄改变被检查表达式。

成员优先可以用两个重载表达：

```cpp
template<class T>
    requires member_readable<T>
decltype(auto) operator()(T&& object) const;

template<class T>
    requires (!member_readable<T> && adl_readable<T>)
decltype(auto) operator()(T&& object) const;
```

第二个重载显式排除 member 路径。这样当一个类型同时有成员和 ADL 函数时，成员重载胜出，而不是让两个模板重载同时可行后依赖更复杂的偏序规则。

## hidden friend 是 ADL 路径的常见形态

自由函数不一定写在类外。它也可以写成类内 friend：

```cpp
namespace model {
struct Hidden {
    int value{};

    friend int& read_value(Hidden& object) noexcept {
        return object.value;
    }
};
}
```

这个 friend 通常不能被普通限定名查找直接找到，例如你不能指望写 `model::read_value(h)` 一定成立；但它能通过 ADL 参与候选集。很多运算符和扩展函数都用这种 hidden friend 形态，因为函数贴近类型定义，同时不污染 namespace 的普通查找结果。

CPO 的 ADL 分支必须支持 hidden friend。若实现只尝试 `object.read_value()`，或者写成 `some_namespace::read_value(object)`，都会漏掉这类合法扩展。

## 不可调用成员不是硬错误

成员存在不等于成员可调用。例如：

```cpp
struct NeedsArgument {
    int value{};
    int read_value(int) const;
};
```

它有一个叫 `read_value` 的成员，但零参数调用不合法。正确 CPO 对 `NeedsArgument{}` 应该判断 member 路径不成立，再尝试 ADL；若 ADL 也没有，就整体不可调用。它不应该因为“看见同名成员”就在函数体里硬调用，导致不受约束的模板实例化错误。

更关键的正例是“坏成员存在，但合法 ADL 也存在”：

```cpp
namespace app_model {
struct Field {
    int value{};
    int read_value(int) const;
};

int& read_value(Field& field) noexcept {
    return field.value;
}
}
```

这里成员名会干扰人读代码，但它不是合法的零参数路径。正确 CPO 必须继续走 ADL，并且 ADL 返回的仍是 `field.value` 的引用，`noexcept` 也来自这个 ADL 函数。练习 checker 专门覆盖这个形态：它检查 `c04::read_value(field)` 可调用、返回 `int&`、写回同一个对象，并且表达式是 `noexcept`。

这就是 `requires` 放在候选边界上的价值。错误发生在“这个候选是否存在”阶段，而不是发生在已经选择函数之后的函数体深处。对用户来说，诊断会更接近“没有满足约束的 `c04::read_value`”，而不是一屏内部实现栈。

## 练习目标

练习 L11 要你实现 `src/student/read_value.hpp`。公开接口固定为：

```cpp
namespace c04 {
inline constexpr /* function object */ read_value{};
}
```

调用形式固定为 `c04::read_value(object)`。实现必须满足：

成员 `object.read_value()` 可调用时优先调用成员；成员不可调用时才尝试 ADL `read_value(object)`；无合法路径时约束拒绝；返回类型用 `decltype(auto)` 保留真实引用和值类别；`noexcept` 与实际选择的表达式一致；内部 ADL 查找不能递归调用 CPO 自己。

checker 覆盖普通左值、`const` 左值、右值、move-only 返回、成员与 ADL 共存、成员不可调用但合法 ADL 存在、成员存在且无合法 ADL、hidden friend、抛异常与 `noexcept`、reference identity，以及无路径对象不可调用。Student 占位实现会成功编译，但会因为 fallback 过宽和不保留引用被 checker 拒绝。

初稿只记录了正确 baseline 和最终 Reference/good/bad 结果，缺少这些前置错误版本的独立观察；本轮补齐 `counterexamples.cpp` 后，教学链条才满足“先看到原问题，再接受下一层机制”的要求。

本章只建立这个小 CPO。更通用的 CPO 家族、`tag_invoke` 争议、Ranges CPO、sender/receiver completion signatures、反射生成字段访问器，会在后续章节把同一条线继续展开。
