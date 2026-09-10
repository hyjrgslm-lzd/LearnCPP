# 08 模块 E：CPO 与 niebloid

## 模块目标

前七个文件（心智模型 + 模块 A/B/C1/C2/D + 结课项目一）带你把 ranges 的使用层走完：管道组合、惰性求值、borrowed_range、projection、C++23 高阶视图。从这个模块开始进入**第二阶段：实现层**。

第二阶段的第一个问题是：`std::ranges::begin`、`std::ranges::sort` 这些东西到底是什么——为什么它们不是普通的函数或函数模板，而是某种"可以被传递的对象"？

本模块聚焦以下四个问题：

- `ranges::begin/end/size/empty/data/rbegin/rend` 这些 CPO（Customization Point Object）是怎么实现的，它们为什么不是函数模板？
- 什么是 niebloid？它和 ranges 访问 CPO、算法函数对象的关系是什么？
- ranges 算法 niebloid 如何做到 ADL 隔离，又为什么能被当作函数对象传递？它和用户可定制访问 CPO 有什么边界？
- 与 stdexec `tag_invoke` 相比，ranges CPO 采用了什么不同的定制策略，各自的设计权衡是什么？

反向引用 `01-心智模型.md` 的"CPO / niebloid"段：那里已经建立了基本直觉——`ranges::begin` 是函数对象而不是函数模板，目的是 ADL 隔离。本模块从那个直觉出发，向下挖到实现层，弄清楚这套机制是怎么构造出来的。

反向引用 `05-模块C2-算法·投影·范围边界.md` 的 niebloid 描述：C2 指出 `ranges::sort` 是 niebloid，不被 ADL 劫持，可作为一等值传递。本模块解释这个性质的实现基础——为什么函数对象具有这两种能力而函数模板没有。

## 模块完成标准

做完本模块，你至少要能稳定说清楚：

- `std::ranges::begin` 不是函数模板，而是 `inline constexpr` 变量——一个带 `operator()` 的函数对象实例。`using std::ranges::begin; begin(r)` 不会走 ADL（因为名字查找找到的是一个变量，变量不参与函数 ADL 重载集构建），这就自然屏蔽了 ADL 污染。
- CPO 的三阶查找逻辑：成员 `.begin()` 优先 → ADL 自由 `begin(r)` fallback → SFINAE 失败拒绝调用。"自由函数 `begin` 仅在返回 iterator 时有效"不是口头说说，而是通过 concept 约束在编译期强制保证的。
- niebloid 和 CPO 的边界：`ranges::begin` 这类访问 CPO 是用户可通过成员/ADL 参与的定制点；`ranges::sort` 这类 ranges 算法 niebloid 是标准库算法函数对象，提供 ADL 隔离、约束重载和 projection，但不是用户可通过 ADL 替换算法体的定制点。C++20/23 已有 ranges 算法函数对象；C++26 P3136 讨论的是把更多标准算法以一等函数对象形式暴露。
- 为什么算法 niebloid 能被传递（`auto f = std::ranges::sort;` 合法），而 `std::sort` 不能（函数模板必须带参数推导，取地址需要显式实例化才能获得函数指针）。
- ranges CPO（C++20）和 stdexec `tag_invoke`（P2300/P1895）的差异：ranges 采用"成员优先 + ADL fallback + 约束过滤"；`tag_invoke` 进一步抽象为单一 ADL 入口 + 标签类型分发，所有定制通过 `friend tag_invoke(tag_t, args...)` 集中注册。

## 使用建议

- 本模块不依赖任何 ranges view 操作，全部练习都在纯 C++20 编译期完成。
- 建议把 "函数模板版本 vs CPO 版本" 并排写在同一文件里，通过 ADL 行为对比加深印象。
- 练习 E-3 涉及 stdexec 概念对照，如果你尚未接触过 `C10_Execution`，可以先读 `P:\C++Code\C10_Execution\08-模块E-定制化机制.md` 的练习 E-1 和 E-2 段落，那里详细解释了 `tag_invoke` 的设计动机和模式。

---

## 前置概念速查

在进入练习之前，先把三个核心定义钉住。

### 什么是 CPO

CPO（Customization Point Object）是一个 `inline constexpr` 全局变量，其类型带有 `operator()`。因为它是对象（变量），不是函数或函数模板，所以：

```cpp
// C++20
// std::ranges::begin 的真实形式（伪实现，展示结构）：
namespace std::ranges {
    namespace _begin_impl {
        struct _begin_fn {
            template<class R>
            constexpr auto operator()(R&& r) const
                /* -> 依据三阶查找确定返回类型 */;
        };
    }
    inline constexpr _begin_impl::_begin_fn begin{};
    //                                      ^^^^^^^^ 这是对象，不是函数
}
```

调用 `std::ranges::begin(r)` 实际上是 `std::ranges::begin.operator()(r)`。因为 `std::ranges::begin` 是变量，名字查找找到的是一个对象，ADL 机制只对函数名生效，对对象名不生效。

### CPO 的三阶查找逻辑

以 `ranges::begin` 为例，其 `operator()` 内部的优先级是：

1. **成员函数优先**：如果 `r.begin()` 合法且返回类型满足 `input_or_output_iterator` concept，则调用 `r.begin()`。
2. **ADL fallback**：否则，在 `decay_t<decltype(r)>` 所在的命名空间里查找 `begin(r)`（ADL），如果找到且返回 `input_or_output_iterator`，则调用它。
3. **SFINAE 失败**：否则，整个 `operator()` 调用是 ill-formed。

```cpp
// C++20（概念示意，非标准源码）
template<class R>
constexpr auto operator()(R&& r) const {
    if constexpr (/* r.begin() 合法且返回 iterator */) {
        return r.begin();
    } else if constexpr (/* ADL begin(r) 合法且返回 iterator */) {
        // 注意：这里的 ADL 查找发生在 CPO 内部，
        // 是受控的——它只查找 decay_t<R> 所在命名空间，
        // 不会因外部 using 声明而扩散
        using std::begin;   // 允许回退到 std::begin（数组支持等）
        return begin(r);
    }
    // else: 不参与重载，调用点 SFINAE 失败
}
```

### 什么是 niebloid

niebloid 是 ranges 约束算法函数对象的社区称呼（名字来自 Eric Niebler）。`ranges::begin/end/size` 等是访问 CPO，用户类型可以通过成员或 ADL fallback 参与定制；`ranges::sort`、`ranges::find`、`ranges::transform` 等是标准库算法对象，调用点有 ADL 隔离、约束和 projection，但用户不能通过同名 ADL 函数替换算法体。

两类名字都常以 `inline constexpr` 函数对象暴露，都能屏蔽调用点 ADL。差异在定制语义：访问 CPO 的目标就是开放受约束的用户定制；算法 niebloid 的目标是提供标准算法的一等 callable 入口，而不是开放用户定制算法体。

| 类别 | 代表 | ADL 隔离 | 可传递 | 算法逻辑封装 |
|------|------|----------|--------|--------------|
| 访问 CPO | `ranges::begin` | 是 | 是 | 是（三阶查找，开放受约束定制） |
| 算法 niebloid | `ranges::sort` | 是 | 是 | 是（标准算法对象，不开放 ADL 替换算法体） |
| 函数模板 | `std::sort` | 否 | 需显式实例化 | — |

---

## 练习 E-1：手写一个简化版 `ranges::begin` CPO

### 目标

亲手实现一个只负责"成员优先 + ADL fallback"的 `my_begin` CPO，通过三个测试场景（成员版本、ADL 版本、无 begin 路径）感受 CPO 如何控制查找策略，并与函数模板版本的 ADL 行为做对比。

### 前置理解

- **为什么变量不参与 ADL**：ADL（Argument-Dependent Lookup）的触发条件是"对函数名的非限定查找"。当 `begin` 是一个变量（对象），名字查找在第一步就找到了这个变量，不会进入 ADL 阶段查找关联命名空间里的函数。这是 CPO 屏蔽 ADL 污染的根本原因，不是什么魔法，而是语言名字查找规则的直接推论。
- **`inline constexpr` 变量**：C++17 引入 `inline` 变量，允许在头文件里定义 `inline constexpr` 全局变量而不违反 ODR。CPO 就是这样的变量，可以安全地放在头文件里被多个翻译单元包含。
- **C++20 concept**：在 CPO 的 `operator()` 里用 `if constexpr` + `requires` 表达式做三阶查找比用 `decltype` 尾置返回类型更清晰，编译错误信息也更易读。

### 预计练习方向

**基础任务：实现 my_begin CPO**

```cpp
// C++20
#include <iterator>
#include <concepts>
#include <vector>
#include <iostream>

namespace my_ranges {

    namespace _my_begin_impl {

        // 辅助 concept：检测 r.begin() 是否合法且返回 iterator
        template<class R>
        concept has_member_begin =
            requires(R& r) {
                { r.begin() } -> std::input_or_output_iterator;
            };

        // 辅助 concept：检测 ADL begin(r) 是否合法且返回 iterator
        // 注意：这里的检测在 _my_begin_impl 命名空间内进行，
        // 确保 ADL 只查找 R 所在命名空间，不查找当前命名空间
        template<class R>
        concept has_adl_begin =
            !has_member_begin<R> &&
            requires(R& r) {
                { begin(r) } -> std::input_or_output_iterator;
            };

        struct my_begin_fn {
            // 成员 begin 路径
            template<class R>
                requires has_member_begin<R>
            constexpr auto operator()(R& r) const
                noexcept(noexcept(r.begin()))
            {
                return r.begin();
            }

            // ADL begin 路径（仅当成员路径不可用时）
            template<class R>
                requires has_adl_begin<R>
            constexpr auto operator()(R& r) const
                noexcept(noexcept(begin(r)))
            {
                return begin(r);
            }

            // 无 begin：没有匹配的 operator() 重载 → 调用点 SFINAE 失败
        };

    } // namespace _my_begin_impl

    // CPO 本体：inline constexpr 变量
    inline constexpr _my_begin_impl::my_begin_fn my_begin{};

} // namespace my_ranges


// ---- 测试场景 1：标准容器——走成员 begin 路径 ----
void test_member_begin() {
    std::vector<int> v = {1, 2, 3};
    auto it = my_ranges::my_begin(v);
    std::cout << "member begin: *it = " << *it << '\n';  // 1
}


// ---- 测试场景 2：只有 ADL begin 的自定义类型 ----
namespace lib_custom {

    struct WithFreeBegin {
        int arr[3] = {10, 20, 30};
        // 没有 member begin()，只有 ADL 自由函数
    };

    // 自由函数 begin 在 lib_custom 命名空间里，ADL 能找到它
    int* begin(WithFreeBegin& w) { return w.arr; }
    int* end(WithFreeBegin& w)   { return w.arr + 3; }

} // namespace lib_custom

void test_adl_begin() {
    lib_custom::WithFreeBegin w;
    auto it = my_ranges::my_begin(w);
    std::cout << "ADL begin:    *it = " << *it << '\n';  // 10
}


// ---- 测试场景 3：无 begin——编译期 SFINAE 失败 ----
void test_no_begin() {
    struct NoBegan { int x; };
    NoBegan nb{42};

    // 下面这行会导致编译错误，因为 NoBegan 既无成员 begin 也无 ADL begin
    // my_ranges::my_begin(nb);  // error: no matching overload

    // 验证方式：用 requires 表达式在运行时检测（不触发错误）
    constexpr bool ok = requires { my_ranges::my_begin(nb); };
    static_assert(!ok, "NoBegan should not have my_begin");
    std::cout << "no-begin path: correctly rejected at compile time\n";
}


// ---- 对比：函数模板版本为何不能做等价约束 ----
//
// 假设我们把 my_begin 写成函数模板而不是 CPO：
//
//   namespace my_ranges_bad {
//       template<class R>
//       constexpr auto my_begin(R& r) { return r.begin(); }
//   }
//
// 问题 1：ADL 污染
//   using my_ranges_bad::my_begin;
//   my_begin(r);
//   // 此时如果 r 所在的命名空间恰好也有一个 my_begin 函数，
//   // 编译器会把它加入候选集，可能产生歧义或意外调用
//
// 问题 2：无法被传递
//   auto f = my_ranges_bad::my_begin;  // 错误：函数模板不能取地址（除非显式实例化）
//   // 而 CPO 版本：
//   auto g = my_ranges::my_begin;      // 合法：my_begin 是一个 constexpr 对象


int main() {
    test_member_begin();
    test_adl_begin();
    test_no_begin();
    return 0;
}
```

### 进阶任务

**给 CPO 加 noexcept 传播**

上面的基础实现已经演示了 `noexcept(noexcept(r.begin()))` 的模式——把被调用表达式的 `noexcept` 性质透传给 CPO 本身。这是标准库 CPO 实现的一个共同特征：调用者可以通过 `noexcept(my_ranges::my_begin(r))` 准确查询底层调用是否抛异常，不会因为 CPO 这一层而丢失 noexcept 信息。

**加 borrowed_range 约束**

标准的 `ranges::begin` 对右值实参有额外限制：只有当右值满足 `enable_borrowed_range` 时才允许调用（防止对临时对象取 begin 后立刻悬垂）：

```cpp
// C++20
template<class R>
    requires (std::is_lvalue_reference_v<R> ||
              std::ranges::enable_borrowed_range<std::remove_cvref_t<R>>)
constexpr auto operator()(R&& r) const { /* ... */ }
```

把这个约束加到 `my_begin` 里，再测试：

- `my_begin(v)` 对左值 `vector`：通过（左值引用）
- `my_begin(std::vector<int>{1,2,3})` 对右值 `vector`：编译期拒绝（vector 未特化 `enable_borrowed_range`）
- `my_begin(std::string_view{"hello"})` 对右值 `string_view`：通过（`string_view` 特化了 `enable_borrowed_range = true`）

**对比 std::begin 函数模板**

```cpp
// C++17 std::begin 是函数模板，无法加等价约束：
// template<class C> constexpr auto begin(C& c) -> decltype(c.begin());
// 它对右值的约束靠重载区分（有 const& 重载），但无法统一地要求"borrowed"语义。
// 更重要的：std::begin 这个函数名会参与 ADL，
// 任何命名空间里的同名函数都可能被意外选中。
```

### 验收点

- 你能用运行结果证明 `my_begin` 在成员版本和 ADL 版本上都正确工作。
- 你能用 `static_assert(!requires { my_ranges::my_begin(nb); })` 证明无 begin 的类型被编译期拒绝。
- 你能写出"将 `my_begin` 改成函数模板后，`using my_ranges_bad::my_begin; my_begin(r);` 与 CPO 版本的 ADL 行为差异"的分析。
- 你能说出 `inline constexpr` 在此处的必要性：`inline` 允许头文件多定义，`constexpr` 允许在常量表达式上下文中调用。
- 你能解释为什么 `has_adl_begin` concept 定义必须放在 `_my_begin_impl` 命名空间内部（而不是 `my_ranges` 命名空间），才能保证 ADL 查找的边界正确。

### 观察点

- CPO 的三阶查找是 `if constexpr` 在语义上的体现：每一阶都是一个 `requires` 约束，只有满足当前阶约束才进入该路径，否则透明地跳到下一阶。这不是运行时分支，而是编译期多态。
- `inline constexpr _begin_fn begin{};` 这行是整套机制的枢纽。它把一个函数对象实例化为一个有名字的全局变量，名字查找找到变量而不是函数，ADL 就此被隔离。
- 实际标准库的 `ranges::begin` 实现比上面的示例复杂：它还要处理数组类型（数组的 `begin` 是 `array + 0`）、`ranges::begin` 不能应用于右值非 borrowed_range 的约束，以及 `[[nodiscard]]` 标记等。核心思路是一致的。

### 常见坑

- **把 `my_begin` 写成普通函数模板**：这样就失去了 ADL 屏蔽效果，把问题带回到出发点。
- **把 `has_adl_begin` concept 放在 `my_ranges` 命名空间**：`requires { begin(r); }` 的 ADL 会查找 `my_ranges` 命名空间本身，可能把 `my_begin` 自身的 `operator()` 算进来，逻辑混乱。
- **忘记 `!has_member_begin<R>` 的前提条件**：如果一个类型同时有成员 begin 和 ADL begin，不加这个前提会让两个重载都参与，产生歧义。
- **误以为 `using my_ranges::my_begin; begin(r)` 会找到 CPO**：`using` 引入的是变量名 `my_begin`，你调用的是 `my_begin(r)` 而不是 `begin(r)`。`begin(r)` 仍然是对自由函数名 `begin` 的 ADL 查找，与 CPO 无关。

### 复盘问题

- `std::ranges::begin` 是一个 `inline constexpr` 变量，那它有"地址"吗？能对它取地址（`&std::ranges::begin`）吗？如果能，类型是什么？
- 假设有两个命名空间 `ns1` 和 `ns2`，各自有一个 `begin(T&)` 自由函数，参数类型 `T` 相同。当 CPO 的 `operator()` 内部做 ADL 查找时，会发生什么？（提示：ADL 查找是否会找到两个候选？）
- CPO 的成员优先策略和 stdexec 的 member-first dispatch（P2855 方向）有何相似之处？又有何不同？

### 对应官方参考

- P0896R4 §22.7（cppreference `std::ranges::begin`）
- Eric Niebler 博文 "Customization Point Design in C++11 and Beyond"
- N4381: Suggested Design for Customization Points

---

## 练习 E-2：算法 niebloid 与函数对象传递

### 目标

用 niebloid 做函数对象传递，亲手对比 `std::ranges::sort` 与 `std::sort` 在"可传递性"上的根本差异，并用一个泛型包装器展示 niebloid 作为函数对象参数的实用价值。反向引用 `05-模块C2-算法·投影·范围边界.md` 的 niebloid 段落——C2 告诉你"它可以传递"，本练习告诉你"它为什么可以传递，以及怎么用"。

### 前置理解

- **函数模板无法直接取地址**：`std::sort` 是一个函数模板，你不能写 `auto f = std::sort;`——因为 `std::sort` 是一个重载集合的名字，不是一个具体的实体。只有当你明确地说 `static_cast<void(*)(Iter,Iter)>(std::sort)` 或 `std::sort<Iter, Iter>` 时，才能得到一个函数指针。这在泛型上下文里非常麻烦。
- **niebloid 是对象**：`std::ranges::sort` 是一个 `inline constexpr` 变量，它有一个固定的类型（某个实现定义的函数对象类型），可以被赋值给 `auto`，可以被传给 `template<auto F>` 参数，可以被 `std::invoke` 调用。这正是函数对象作为一等公民的优势。
- **projection 在 niebloid 里是 first-class 参数**：`ranges::sort(v, comp, proj)` 的签名在 niebloid 内部统一处理，调用者不需要手写 lambda 包装。这一点在 C2 模块的 projection 练习中已经直接使用过。

### 预计练习方向

**基础任务：niebloid 可传递 vs 函数模板不可传递**

```cpp
// C++20
#include <algorithm>
#include <ranges>
#include <vector>
#include <iostream>
#include <string>
#include <functional>

// ---- 1. 直接赋值 ----
void demo_assignability() {
    // niebloid：合法
    auto sort_fn = std::ranges::sort;
    // sort_fn 的类型是实现定义的，但它是一个可调用对象
    // 可以被直接调用：
    std::vector<int> v = {3, 1, 4, 1, 5};
    sort_fn(v);
    std::cout << "sorted: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';  // 1 1 3 4 5

    // 函数模板：不合法（必须显式实例化或用 lambda 包装）
    // auto sort_bad = std::sort;     // 编译错误：无法从重载集取值
    // 需要显式实例化才能得到函数指针：
    using Iter = std::vector<int>::iterator;
    auto sort_explicit = static_cast<void(*)(Iter, Iter)>(std::sort);
    // 这样拿到的是一个具体重载的函数指针，失去了泛型性
}


// ---- 2. 作为 template<auto F> 非类型模板参数传递 ----
template<auto Algo, class R, class... Args>
void apply_algo(R&& r, Args&&... args) {
    Algo(std::forward<R>(r), std::forward<Args>(args)...);
}

void demo_template_param() {
    std::vector<int> v = {5, 3, 8, 1, 9};

    // 直接把 niebloid 作为非类型模板参数：
    apply_algo<std::ranges::sort>(v);
    std::cout << "apply_algo sorted: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';  // 1 3 5 8 9

    // 带自定义比较器：
    apply_algo<std::ranges::sort>(v, std::greater<>{});
    std::cout << "apply_algo sorted desc: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';  // 9 8 5 3 1
}


// ---- 3. 带 projection 对 vector<Person> 按年龄排序 ----
struct Person {
    std::string name;
    int age;
};

void demo_projection_sort() {
    std::vector<Person> people = {
        {"Bob",   30},
        {"Alice", 25},
        {"Carol", 35},
        {"Dave",  28},
    };

    // niebloid + projection：可以直接传
    auto sort_by_age = std::ranges::sort;
    sort_by_age(people, std::less{}, &Person::age);

    std::cout << "sorted by age: ";
    for (const auto& p : people)
        std::cout << p.name << '(' << p.age << ") ";
    std::cout << '\n';  // Alice(25) Dave(28) Bob(30) Carol(35)

    // 对比：用 std::sort 做同样的事，需要包 lambda：
    // std::sort(people.begin(), people.end(),
    //           [](const Person& a, const Person& b){ return a.age < b.age; });
    // std::sort 本身无法直接传为 auto，更无法在泛型上下文里统一 projection 接口
}


int main() {
    demo_assignability();
    demo_template_param();
    demo_projection_sort();
    return 0;
}
```

### 进阶任务

**泛型包装器对比代码长度和可读性**

```cpp
// C++20
#include <algorithm>
#include <ranges>
#include <vector>
#include <functional>
#include <iostream>

// ---- 泛型算法包装器 ----
// 要求：Algo 是可调用对象（niebloid 或 lambda 包装的 std:: 算法）

// 版本 A：用 niebloid 直接传
template<auto Algo, class R, class Comp = std::ranges::less, class Proj = std::identity>
void sorted_print(R r, Comp comp = {}, Proj proj = {}) {
    Algo(r, comp, proj);
    for (const auto& x : r)
        std::cout << x << ' ';
    std::cout << '\n';
}

// 版本 B：用 lambda 包装 std::sort（不能直接传）
template<class SortFn, class R, class Comp = std::less<>>
void sorted_print_legacy(SortFn sort_fn, R r, Comp comp = {}) {
    sort_fn(r.begin(), r.end(), comp);
    for (const auto& x : r)
        std::cout << x << ' ';
    std::cout << '\n';
}

int main() {
    std::vector<int> v = {5, 2, 8, 1, 9, 3};

    // 版本 A：传 niebloid，简洁
    sorted_print<std::ranges::sort>(v);                    // 升序
    sorted_print<std::ranges::sort>(v, std::greater<>{});  // 降序

    // 版本 B：必须手工包装 std::sort
    auto wrapped_sort = [](auto first, auto last, auto comp) {
        std::sort(first, last, comp);
    };
    sorted_print_legacy(wrapped_sort, v);

    // 观察：
    // - 版本 A 的调用点不需要知道 Algo 的签名细节，projection 也可以扩展
    // - 版本 B 需要一个额外的 lambda wrapper，且必须固定迭代器参数形式
    // - 当算法需要 projection 时，版本 B 的 wrapper 会更复杂

    return 0;
}
```

**niebloid 的内部实现示意**

理解 niebloid 为什么不被 ADL 找到的另一种视角：

```cpp
// C++20（概念示意，非标准源码）
namespace std::ranges {
    namespace _sort_impl {
        struct _sort_fn {
            // 完整的算法实现都在 operator() 里
            template<
                std::random_access_iterator I,
                std::sentinel_for<I> S,
                class Comp = ranges::less,
                class Proj = std::identity
            >
            requires std::sortable<I, Comp, Proj>
            constexpr I operator()(I first, S last,
                                   Comp comp = {}, Proj proj = {}) const;

            // range 重载版本
            template<
                ranges::random_access_range R,
                class Comp = ranges::less,
                class Proj = std::identity
            >
            requires std::sortable<ranges::iterator_t<R>, Comp, Proj>
            constexpr ranges::borrowed_iterator_t<R>
            operator()(R&& r, Comp comp = {}, Proj proj = {}) const;
        };
    }

    // niebloid 本体
    inline constexpr _sort_impl::_sort_fn sort{};
}

// 为什么这样设计后 sort 不被 ADL 找到：
//   namespace user_ns {
//       struct Foo {};
//   }
//   using std::ranges::sort;
//   sort(user_ns_foo);   // 名字查找找到变量 sort，不发起函数名 ADL
//                        // 不会在 user_ns 里查找同名函数
```

### 验收点

- 你能用代码证明 `auto f = std::ranges::sort;` 编译通过，而 `auto g = std::sort;` 编译失败（或需要显式实例化）。
- 你能实现 `apply_algo<std::ranges::sort>(v)` 的 `template<auto Algo>` 包装，并展示与 lambda 包装 `std::sort` 的代码长度对比。
- 你能用 `std::ranges::sort` 加 projection 对 `vector<Person>` 完成按年龄排序，且不用任何手写 lambda。
- 你能解释为什么 niebloid 名字不会被 ADL 找到，而 `std::sort` 的名字会参与 ADL。
- 你能说出访问 CPO 与算法 niebloid 的准确边界：`ranges::begin` 开放成员/ADL 定制；`ranges::sort` 是可传递的标准算法对象，不是用户可 ADL 替换的算法定制点。

### 观察点

- "函数对象可以被传递"听起来是小事，但在泛型编程里意义重大：当你写 `template<auto Algo>` 时，每个不同的 `Algo` 实例化都产生不同的特化版本，编译器可以完全内联（消除函数调用开销）。用函数指针（`std::sort` 的显式实例化结果）则通常不能被内联。
- `ranges::sort` 的两个 `operator()` 重载（迭代器版和 range 版）都在同一个类里。这使得 `ranges::sort(v)` 和 `ranges::sort(v.begin(), v.end())` 都通过同一个 niebloid 分发，接口统一，而 `std::sort` 的这两种调用形式是不同重载集的不同函数。
- niebloid 的名字不是标准术语——C++ 标准文本里称其为"范围算法定制点对象"或不作命名。"niebloid"是社区约定的称呼，来自 Eric Niebler 的设计。

### 常见坑

- **期待 `auto f = std::sort;` 编过**：这不会发生。函数模板是一个重载集合，不是一个可赋值的实体。必须 `std::sort<Iter, Iter>` 或用 `static_cast` 才能得到函数指针，而这会丢失泛型性。
- **以为 niebloid 和 `std::` 算法签名完全一致，可以互相替代**：不是的。`ranges::sort` 只在 `std::ranges::` 命名空间下，签名包含 projection 参数，返回 `borrowed_iterator_t<R>` 而非 `void`。`std::sort` 没有 projection，返回 `void`，要求 begin/end 同类型。
- **以为 `using std::ranges::sort; sort(v)` 会走 ADL**：不会。`using` 引入的是变量 `sort`（一个对象），变量名查找不触发 ADL。这正是 niebloid 的设计目的。
- **混淆 `std::ranges::sort` 的类型和它的值**：类型是 `std::ranges::_sort_impl::_sort_fn`（实现定义），值是 `std::ranges::sort` 这个 `inline constexpr` 实例。你需要的是这个值，不是这个类型（通常用 `auto` 接收即可）。

### 复盘问题

- `std::ranges::sort(v)` 返回的不是 `void` 而是 `ranges::borrowed_iterator_t<R>`，这个返回值代表什么？在什么场景下会用到它？
- 如果你写一个接受泛型算法的函数 `void apply(auto algo, auto& range)`，把 `std::ranges::sort` 传进去是否比 lambda 包装 `std::sort` 更好？在什么维度上更好？
- niebloid 能被存入 `std::function` 吗？如果能，有什么代价？如果不能，为什么？

### 对应官方参考

- P0896R4（ranges 算法 niebloid 设计）
- cppreference [`std::ranges::sort`](https://en.cppreference.com/w/cpp/algorithm/ranges/sort)
- cppreference [Niebloids（ranges constrained algorithms）](https://en.cppreference.com/w/cpp/algorithm/ranges)

---

## 练习 E-3：ranges CPO 与 stdexec tag_invoke 对照

### 目标

横向对照 ranges CPO（C++20）和 stdexec `tag_invoke`（P1895R0/P2300）这两套定制机制的设计演进，理解各自的适用场景和权衡，并通过给同一个类型同时注册两套定制点的练习，感受它们在"如何扩展"上的具体差异。

**注意**：`tag_invoke` 不是 C++ 标准，它来自 stdexec/P2300 提案，即便最终 C++26 可能采纳某种形式的执行库，`tag_invoke` 目前也只是提案中的设计模式，不是任何已发布标准的一部分。下面的对照是理解演进思路，不是说"标准要求两套并用"。

### 前置理解

- **ranges CPO 的定制模式**（你刚在 E-1 实现过）：三阶查找，以成员函数优先，ADL 自由函数为 fallback，每个 CPO 各自独立定义自己的三阶查找逻辑。用户通过"给类型加成员 `begin()`"或"在类型所在命名空间加 ADL `begin(r)`"接入。
- **`tag_invoke` 的定制模式**（来自 stdexec/P2300）：所有定制点共享同一个 ADL 函数名 `tag_invoke`，通过第一个参数（标签类型 tag）区分不同的定制点。用户通过 `friend tag_invoke(connect_t, MyType, ...)` 在类体内部声明 friend 函数来接入。分发链是：`cpo(args)` → `tag_invoke(cpo_tag, args)` → 用户的 `friend` 实现。
- **为什么需要对照**：ranges 先于 stdexec，以"每个 CPO 各自管理自己的 ADL"方式实现。stdexec 面对更多的定制点（connect、start、set_value、get_env……），发现"N 个独立 ADL 函数名"仍有潜在冲突风险，于是进一步收窄为"1 个 ADL 名字（`tag_invoke`）+ N 个 tag 类型"。两者都是对 ADL 污染的防御，但抽象层次不同。

### 预计练习方向

**两套机制的查找流程对比（概念示意）**

```
ranges CPO 查找流程（以 ranges::begin 为例）：
┌─────────────────────────────────────────────────────┐
│  调用: ranges::begin(r)                              │
│  ↓ CPO operator() 内部                              │
│  阶段1: r.begin() 是否合法且返回 iterator?           │
│    是 → 调用 r.begin()，结束                         │
│    否 ↓                                              │
│  阶段2: ADL begin(r) 是否合法且返回 iterator?        │
│    是 → 调用 begin(r)（查找 decay_t<R> 所在命名空间）│
│    否 ↓                                              │
│  阶段3: SFINAE 失败，调用点编译错误                  │
└─────────────────────────────────────────────────────┘

tag_invoke 查找流程（以 stdexec::connect 为例）：
┌─────────────────────────────────────────────────────┐
│  调用: connect(sender, receiver)                     │
│  ↓ connect CPO operator() 内部                      │
│  tag_invoke(connect_t{}, sender, receiver)           │
│  ↓ 单一 ADL 入口（只有 tag_invoke 这一个名字）      │
│  查找 sender/receiver 所在命名空间的 tag_invoke 重载 │
│  ↓                                                   │
│  friend tag_invoke(connect_t, MySender, Receiver)    │
│  （用户在自己的类体内定义的 friend 函数）            │
└─────────────────────────────────────────────────────┘
```

**四维度对照表**

```cpp
// C++20（代码注释形式的对照表）

// 维度 1：如何新增定制点（以"让 MyContainer 支持 ranges::begin"为例）

// ——ranges CPO 方式：添加成员函数 begin()——
struct MyContainer_RangesCPO {
    int data[5] = {1, 2, 3, 4, 5};

    // 选择 1：成员函数（ranges::begin 三阶查找的第一阶）
    int* begin() { return data; }
    int* end()   { return data + 5; }
};
// 就这样。不需要了解 CPO 的内部实现，只需要满足"有 begin() 成员"。

// ——tag_invoke 方式（假设 connect 是我们的定制点）——
struct connect_t {};  // tag 类型
inline constexpr connect_t connect_tag{};

struct MySender_TagInvoke {
    int value;

    // 用户通过 friend tag_invoke 注册定制
    template<typename Receiver>
    friend auto tag_invoke(connect_t, MySender_TagInvoke s, Receiver r) {
        // 返回 operation_state（此处简化）
        return s.value;
    }
};
// tag_invoke 方式需要知道 connect_t 标签类型，模板参数更显式

// 维度 2：如何屏蔽 ADL 污染

// ——ranges CPO：通过变量名屏蔽——
// ranges::begin 是变量，调用 ranges::begin(r) 不触发函数名 ADL
// 但！CPO 内部 fallback 的 ADL 查找仍然存在（受控的 ADL）

// ——tag_invoke：通过单一名字收窄——
// ADL 里只有 tag_invoke 这一个名字。不同定制点用不同 tag 区分。
// 命名空间里有 tag_invoke 冲突的概率比有 begin/connect/start/get_env 全部冲突低得多

// 维度 3：错误消息定位

// ——ranges CPO：错误发生在三阶查找的 SFINAE 失败处——
// "no matching overload for my_begin(NoBegan)"
// 错误点明确（SFINAE 失败在 CPO 内部），但不同 CPO 的错误格式各异

// ——tag_invoke：错误发生在 tag_invoke 查找失败处——
// "no matching function for call to tag_invoke(connect_t, ...)'"
// 所有定制点的错误格式统一（都是 tag_invoke 找不到），更容易识别

// 维度 4：泛型扩展性

// ——ranges CPO：适合"接入标准已有 CPO"——
// 标准库有多少个 CPO，用户就有多少种接入方式；不能自定义新的"CPO 协议"
// （可以写新的 CPO 类，但没有统一的"注册"入口）

// ——tag_invoke：适合"框架级的可扩展定制点"——
// 框架定义新 CPO 只需要新建一个 tag 类型；用户接入新定制点的方式与已有定制点完全一致
// 这就是 stdexec 有几十个定制点还能保持接入方式统一的原因
```

**给同一个类型同时注册两套定制（进阶）**

```cpp
// C++20
#include <ranges>
#include <iterator>
#include <iostream>

// 模拟一个 tag_invoke 基础设施（简化版，仅用于演示）
namespace my_exec {
    struct connect_t {};
    inline constexpr connect_t connect{};

    // tag_invoke 的 poison pill（阻止无匹配时静默失败）
    namespace _ti_impl {
        void tag_invoke() = delete;  // poison pill
    }

    template<class Tag, class... Args>
    auto tag_invoke_cpo(Tag tag, Args&&... args)
        -> decltype(_ti_impl::tag_invoke(tag, std::forward<Args>(args)...))
    {
        using _ti_impl::tag_invoke;
        return tag_invoke(tag, std::forward<Args>(args)...);
    }
} // namespace my_exec


// MyContainer：同时支持 ranges CPO 和 tag_invoke
class MyContainer {
    int data_[4] = {10, 20, 30, 40};
public:
    // ---- 接入 ranges CPO（通过成员 begin/end）----
    int* begin() { return data_; }
    int* end()   { return data_ + 4; }
    std::size_t size() const { return 4; }

    // ---- 接入 tag_invoke（通过 friend 声明）----
    // 假设 connect 的意义是"获取容器的第一个元素"（仅演示用）
    friend int tag_invoke(my_exec::connect_t, MyContainer& c) {
        return c.data_[0];
    }
};


void demo_dual_customization() {
    MyContainer mc;

    // 通过 ranges CPO 访问：
    auto it = std::ranges::begin(mc);
    std::cout << "ranges::begin -> " << *it << '\n';  // 10

    std::size_t sz = std::ranges::size(mc);
    std::cout << "ranges::size  -> " << sz << '\n';   // 4

    // for-range 也走 ranges CPO：
    for (int x : mc)
        std::cout << x << ' ';
    std::cout << '\n';  // 10 20 30 40

    // 通过 tag_invoke 访问（模拟 stdexec 风格）：
    int first = my_exec::tag_invoke_cpo(my_exec::connect, mc);
    std::cout << "tag_invoke connect -> " << first << '\n';  // 10

    // 同一个类型在两套定制机制下注册：
    // - ranges 侧：成员 begin/end/size（三个"接入口"）
    // - tag_invoke 侧：一个 friend tag_invoke（一个"接入口"）
    // 两套机制不冲突，用不同的调用路径分别触发
}


int main() {
    demo_dual_customization();
    return 0;
}
```

### 进阶任务

**range-v3 风格的集中入口（了解扩展方向）**

range-v3 库（ranges 标准化的前身）内部有一种"集中 dispatch 的 CPO"写法，把成员查找和 ADL 查找统一封装成一个 `_cpo_t` 基类的 trait，使得所有 CPO 的模板结构统一。这比标准库里每个 CPO 单独实现三阶查找更整齐，但原理是一样的。你可以阅读 range-v3 的 `include/range/v3/range/access.hpp` 感受这种集中化的写法——它已经与 `tag_invoke` 的思路非常接近。

### 验收点

- 你能画出（或写出注释形式的）ranges CPO 三阶查找和 tag_invoke 单一入口的流程图，并指出两者的关键差异。
- 你能列出四维度对照表（如何新增定制点、如何屏蔽 ADL、错误消息定位、泛型扩展性）并填写 ranges CPO 和 tag_invoke 两列。
- 你能给 `MyContainer` 同时实现 ranges 侧和 tag_invoke 侧的定制，并用代码验证两条路径都能工作。
- 你能清楚说出"`tag_invoke` 不是 C++ 标准，只是 stdexec/P2300 里的提案定制模式"，并说出在哪个提案文件里可以找到它的设计说明（P1895R0）。
- 你能解释 ranges CPO 为什么没有采用 `tag_invoke` 的集中入口（时间线因素：ranges 是 C++20，`tag_invoke` 是 P1895，比 C++20 晚提出且尚未进入标准）。

### 观察点

- ranges CPO 和 `tag_invoke` 解决的是同一个根本问题：ADL 污染和定制点的可扩展性。它们的区别是抽象层次——CPO 每个独立管理，`tag_invoke` 统一收敛。这不是"谁更好"的问题，而是"谁更适合当前场景"。
- 给一个类型同时注册两套机制不仅是可行的，在现实中也有必要——一个容器类型既需要支持 ranges 管道访问，又可能需要作为 stdexec 的 sender 传递。两套机制的接入方式不重叠，互不干扰。
- ranges CPO 目前每个定制点各自独立实现三阶查找，这意味着如果标准库新增一个 CPO，要写完整的一套逻辑。`tag_invoke` 的优势是：新增定制点只需要新建一个 tag 类型，用户的接入方式完全统一，框架侧的代码量大大减少。

### 常见坑

- **混淆访问 CPO 和算法 niebloid**：`ranges::begin/end/size` 是访问 CPO，开放成员/ADL 定制；`ranges::sort/find/transform` 是算法 niebloid，重点是函数对象传递、ADL 隔离、约束和 projection，不开放用户通过同名 ADL 函数替换算法体。
- **以为 `tag_invoke` 是 C++ 标准的一部分**：不是。P1895R0 是一个提案，`tag_invoke` 是 stdexec（NVIDIA 的参考实现）和 libunifex 使用的定制模式。C++26 的 `std::execution` 可能采纳，也可能采用不同形式。不要在需要可移植代码的地方依赖 `tag_invoke`。
- **以为 ranges CPO 不能做集中入口**：ranges 标准库目前的实现是"每个 CPO 各自独立"，但理论上可以用类似 `tag_invoke` 的集中入口重新实现同样的三阶查找——range-v3 就做了部分集中化。"ranges CPO 不集中"是实现选择，不是机制限制。

### 复盘问题

- `tag_invoke` 用一个 ADL 名字 + N 个 tag，比 ranges CPO 用 N 个独立 ADL 查找，在实际大项目里的冲突风险差异有多大？能举出一个 `tag_invoke` 还是会有冲突的场景吗？
- 如果你要从零设计一个新的 C++ 框架，你会选择 ranges 风格的 CPO 还是 `tag_invoke` 风格？基于什么原则做决定？
- ranges CPO 的三阶查找里，"成员优先"这一条和 stdexec 的 member-first dispatch（P2855）有什么相似之处？两者的出现顺序（C++20 ranges vs C++26 方向的 P2855）能说明什么演进规律？
- 在 `MyContainer` 同时注册两套定制的练习里，两套机制的"接入代码量"差异是什么？哪套更容易被第三方库的类型接入（你不拥有源码的情况）？

### 对应官方参考

- P1895R0: tag_invoke: A general pattern for supporting customisable functions
- P2300R10: `std::execution` 提案中 CPO 的定义和 `tag_invoke` 的使用
- P2855R1: Member customization points for Senders and Receivers（member-first 方向）
- cppreference [`std::ranges::begin`](https://en.cppreference.com/w/cpp/ranges/begin)

---

## 做完模块 E 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- `std::ranges::begin` 是 `inline constexpr` 变量，它的类型是一个带 `operator()` 的函数对象类型。`using std::ranges::begin; begin(r)` 找到的是变量 `begin`，不会发起函数名 ADL，这就是 ADL 污染被屏蔽的机制——变量查找优先，且变量名不参与 ADL 候选集构建。
- CPO 内部的三阶查找是编译期的 concept 检测链：成员 `.begin()` → ADL `begin(r)` → SFINAE 失败。每一阶都通过 `requires` 表达式做约束，不是运行时 if。
- niebloid 是 ranges 算法函数对象的社区称呼。`ranges::begin` 是访问 CPO，开放受约束定制；`ranges::sort` 是可传递的标准算法对象，不是用户可 ADL 替换的算法定制点。
- `auto f = std::ranges::sort;` 合法，因为 `ranges::sort` 是一个 `constexpr` 对象，可以被赋值给 `auto`。`auto g = std::sort;` 不合法，因为 `std::sort` 是函数模板，不是可赋值的实体（必须显式实例化）。
- ranges CPO（C++20）和 `tag_invoke`（stdexec P2300）都是对 ADL 污染的系统性修复，但层次不同：ranges CPO 每个独立管理三阶查找，`tag_invoke` 把所有定制收敛到单一 ADL 名字 + 标签分发。前者在 C++20 已标准化，后者目前只在提案和 stdexec 实现中。

**一句话 take-away**：CPO 是 C++20 对 ADL 污染的一次系统性修复；niebloid 是算法层加的一层"能被当作函数对象传递"的礼物；stdexec `tag_invoke` 则是下一代对这套思路的集中化——三个台阶，每一级都在解决上一级暴露出来的问题。

**指向模块 F**：模块 F 进入"概念精化与迭代器分类"——`weakly_incrementable` → `contiguous_iterator` 的 concept 链、`iterator_concept` vs `iterator_category` 双轨（`01-心智模型.md` 已经建立过基础直觉，F 模块从实现层切入）、`sentinel_for`/`sized_sentinel_for`/`iter_value_t`/`iter_reference_t` 等迭代器关联类型的实现原理，以及如何为自定义 iterator 正确设置双轨以支持 C++17 遗留算法兼容。

---

## 映射提案段

| 提案编号 | 内容摘要 |
|----------|----------|
| P0896R4 | C++20 核心合入：ranges CPO 的设计规范（`ranges::begin/end/size` 等的三阶查找规则） |
| P1895R0 | `tag_invoke`：单一 ADL 入口 + 标签类型分发的通用定制模式（stdexec/libunifex 采用，非 C++ 标准） |
| P2855R1 | Member customization points for Senders and Receivers：member-first dispatch 方向，CPO 优先检测成员函数（C++26 执行库方向） |

---

## 本轮实现校准：E1/E2/E3

E1 的实现练习现在以 `c06_e1::my_begin` 为唯一学生接口，checker 覆盖成员 begin、ADL begin、成员优先、数组、右值 borrowed 过滤和无 begin 拒绝。这里的“成员优先 + ADL fallback”不是“随便找一个 begin”：标准 `ranges::begin` 对数组有专门路径，对右值非 borrowed range 会拒绝，以免返回悬垂迭代器。

E2 是观察型练习。`std::ranges::sort` 在 C++20/23 已经是范围算法函数对象，具备 ADL 隔离、约束重载和 projection；C++26 的 P3136 讨论的是更广义的 algorithm function objects 能力补齐。讲解时应按版本分层：C++20/23 的 ranges 算法 niebloid是一类既有标准对象；C++26 的算法函数对象不是把所有算法统称为可定制 CPO。

E3 是观察型练习。ranges CPO 和历史 `tag_invoke` 都在解决“定制点如何开放且不被 ADL 污染”的问题，但时间线不同：ranges CPO 是 C++20 标准机制；`tag_invoke` 是 P1895/stdexec/libunifex 语境中的协议风格，用一个 ADL 名字配多个 tag。这里作为 C04 定制点知识与 C10 execution 协议之间的桥，不把 `tag_invoke` 说成当前 ranges 的实现协议。
