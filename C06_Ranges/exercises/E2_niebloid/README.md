> 对应章节：../../08-模块E-CPO与niebloid.md 练习 E-2：算法 niebloid 与函数对象传递

## 目标

用 niebloid 做函数对象传递，亲手对比 `std::ranges::sort` 与 `std::sort` 在"可传递性"上的根本差异，并用泛型包装器展示 niebloid 作为函数对象参数的实用价值。反向引用模块 C2 的 niebloid 段落——C2 告诉你"它可以传递"，本练习告诉你"它为什么可以传递，以及怎么用"。

## 前置理解

- **函数模板无法直接取地址**：`std::sort` 是函数模板——一个重载集合的名字，不是具体实体。`auto f = std::sort` 无法编译，必须显式实例化（丢失泛型性）才能得到函数指针。
- **niebloid 是对象**：`std::ranges::sort` 是 `inline constexpr` 变量，有固定类型，可赋值给 `auto`，可传给 `template<auto F>` 非类型模板参数，可被 `std::invoke` 调用。这是函数对象作为一等公民的核心优势。
- **niebloid vs CPO 的范围**：`ranges::begin` 是访问 CPO，开放成员/ADL 定制；`ranges::sort` 是 ranges 算法 niebloid，重点是 ADL 隔离、约束和 projection，不是用户可通过 ADL 替换算法体的定制点。
- **projection 是 first-class 参数**：`ranges::sort(v, comp, proj)` 在 niebloid 内部统一处理，调用者无需手写 lambda 包装器。

## 必做任务

1. **验证 niebloid 可赋值**：`auto sort_fn = std::ranges::sort;` 编译通过；用 `sort_fn(v)` 排序并打印验证。对比注释掉的 `auto sort_bad = std::sort;`（编译失败）。

2. **实现 `apply_algo` 包装器**：`template<auto Algo, class R, class... Args> void apply_algo(R&& r, Args&&... args)`，完美转发参数给 `Algo`。演示 `apply_algo<std::ranges::sort>(v)` 和 `apply_algo<std::ranges::sort>(v, std::greater<>{})` 两种调用形式。

3. **projection 排序 `vector<Person>`**：`auto sort_by_age = std::ranges::sort; sort_by_age(people, std::less{}, &Person::age);`——不使用任何手写 lambda，利用 niebloid 的 projection 参数完成按年龄排序。

4. **泛型包装器对比**：实现版本 A（`template<auto Algo>`，niebloid 直接传）和版本 B（接受 lambda 包装的 `std::sort`），对比调用点代码量和可读性。

## 进阶任务

- **`template<auto Algo>` 的编译器内联效果**：用 `template<auto Algo>` 接收 niebloid 时，每个不同的 `Algo` 产生不同特化，编译器可完全内联（消除调用开销）。函数指针通常不能被内联——理解这个维度上的性能差异。
- **niebloid 存入 `std::function`**：尝试 `std::function<void(std::vector<int>&)> f = std::ranges::sort;`，分析是否可行，代价是什么（类型擦除 + 潜在的多态开销）。
- **分析 `ranges::sort` 返回值**：`ranges::sort(v)` 返回 `ranges::borrowed_iterator_t<R>`（指向排序后末尾的迭代器），不是 `void`。理解这个返回值在什么场景下有用。

## 验收点

- 代码证明 `auto f = std::ranges::sort;` 编译通过，`auto g = std::sort;` 需要显式实例化。
- `apply_algo<std::ranges::sort>(v)` 实现完整且运行正确。
- `vector<Person>` 按年龄排序不用任何 lambda。
- 能解释为什么 niebloid 名字不被 ADL 找到，而 `std::sort` 的名字会参与 ADL。
- 能说出访问 CPO 与算法 niebloid 的边界：`ranges::begin` 开放受约束定制；`ranges::sort` 是可传递的标准算法对象。

## 观察点

- `ranges::sort` 的两个 `operator()` 重载（迭代器版和 range 版）在同一个类里。`ranges::sort(v)` 和 `ranges::sort(v.begin(), v.end())` 通过同一个 niebloid 分发——接口统一，而 `std::sort` 的两种调用是不同重载集。
- "niebloid"不是标准术语——C++ 标准文本称其为"范围算法定制点对象"。"niebloid"是社区约定，来自 Eric Niebler 的设计。
- `using std::ranges::sort; sort(r)` 不会走 ADL——`using` 引入的是变量 `sort`（对象），对象名查找不触发函数名 ADL，这正是 niebloid 的设计目的。

## 常见坑

- **期待 `auto f = std::sort` 编过**：不可能。函数模板是重载集合的名字，不是可赋值实体。
- **以为 `ranges::sort` 和 `std::sort` 签名一致**：不是。前者有 projection 参数，返回 `borrowed_iterator_t<R>`；后者无 projection，返回 `void`，要求 begin/end 同类型。
- **以为 `using std::ranges::sort; sort(v)` 走 ADL**：不会。`using` 引入的是变量，不触发函数名 ADL。
- **混淆 `std::ranges::sort` 的类型和值**：类型是实现定义的 `_sort_fn`，值是 `inline constexpr` 实例。用 `auto` 接收值即可，不需要知道类型名。

## 复盘问题

- `std::ranges::sort(v)` 返回 `ranges::borrowed_iterator_t<R>`，这个返回值代表什么？在什么场景下会用到它？
- 如果你写 `void apply(auto algo, auto& range)` 并传入 `std::ranges::sort`，比 lambda 包装 `std::sort` 在哪个维度上更好？
- niebloid 能存入 `std::function` 吗？如果能，有什么代价？如果不能，为什么？

## 对应官方参考

- P0896R4（ranges 算法 niebloid 设计）
- cppreference: [std::ranges::sort](https://en.cppreference.com/w/cpp/algorithm/ranges/sort)
- cppreference: [Niebloids（ranges constrained algorithms）](https://en.cppreference.com/w/cpp/algorithm/ranges)

## Author-validation note

本题是观察型练习，CMake 使用 `ranges_add_observation(E2_niebloid main.cpp)`。当前 `main.cpp` 是完整可运行对照：`std::ranges::sort` 可赋给 `auto`、可作为 `template<auto Algo>` 参数、可携带 projection；`std::sort` 只能通过显式实例化或 lambda 包装变成具体 callable。

术语上需要区分历史：C++20/23 ranges 算法 niebloid 已以函数对象形式提供 ADL 隔离、约束和 projection；C++26 P3136 进一步讨论更多 algorithm function objects 的一等值能力。不要把算法对象一概称为用户可定制 CPO，也不要把 C++26 能力倒灌回 C++20/23 解释。
