> 对应章节：05-模块C2-算法·投影·范围边界 · 练习 C2-1：projection 模式

# C2-1：projection 模式

## 目标

让 projection 成为条件反射，彻底甩掉"每次都写 lambda 捕获 member pointer"的惯性。
练习结束后，看到"对结构体集合排序/查找/统计"时，应立刻想到 projection 而非 lambda 包装。

## 前置理解

- 已在 `01-心智模型.md` 读过 `ranges::sort(employees, std::greater{}, &Employee::salary)`
  的用法，知道 projection 是算法第三个（或第四个）参数。
- 知道 `std::identity` 是恒等映射，是所有 ranges 算法的默认 projection。
- 接受本题的重点是"projection vs lambda"在代码表达力上的差异，
  以及 projection 可以是任意 Callable 这一事实。

## 预计练习方向

1. **成员指针 projection — sort**：
   `ranges::sort(people, std::less{}, &Person::age)` 排序，打印结果。
   在注释里写出等价的 C++17 lambda 版本，对比意图表达力。
2. **成员指针 projection — find**：
   `ranges::find(people, std::string{"Alice"}, &Person::name)` 查找并打印。
3. **成员指针 projection — max**：
   `ranges::max(people, std::less{}, &Person::age)` 取最年长者。
4. **min_max_result 结构化绑定**：
   `auto [youngest, oldest] = ranges::minmax(people, std::less{}, &Person::age)`
   分别打印，体会富返回类型的使用方式。
5. **count_if + projection**：
   `ranges::count_if(people, [](int a){ return a >= 30; }, &Person::age)`
   验证计数为 2（Bob 30, Carol 35）。
6. **lambda 作为 projection**：
   用 `[](const Person& p){ return p.name[0]; }` 作为 projection，
   用 `ranges::find_if` 找到首字母为 `'C'` 的人。

## 进阶预计方向

- 自定义 `Department` 结构体，用 lambda 计算人均预算作为 projection，
  用 `ranges::sort` 按人均预算降序排列，体会 projection 不只限于成员指针。
- 用 `static_assert` 验证 `ranges::min_max_result<int>` 有 `min` 和 `max` 两个成员。
- 探索 `ranges::partial_sort_copy` 的双 projection 参数（in-proj 和 out-proj）。

## 验收点

- 能把 `sort / find / max / count_if / minmax_element` 的 projection 版本与
  C++17 lambda 版本并排写出，说出 projection 在可读性和代码意图上的优势。
- 能用 `static_assert` 验证 `ranges::minmax_result` 的结构（有 `min` 和 `max` 成员）。
- 能说出 projection 的两类合法类型：成员指针和任意 Callable。
- 能解释 `std::identity{}` 作为 projection 的语义。

## 观察点

- projection 的应用位置是算法内部热点：每次需要比较时，算法先对两个元素各自调用
  `proj`，再把结果传给 `comp`。伪代码是 `comp(proj(*it1), proj(*it2))`。
  不存在"先 comp 后 proj"的情况。
- projection 比 lambda 的优势不只是"更短"：成员指针作为 projection 时，
  编译器通常能更好地内联（成员访问是已知偏移量）。
  更重要的是语义分离："`sort` by `age`"与"`sort` where `a.age < b.age`"
  表达的意图精度不同。
- `ranges::count_if(r, pred, proj)` 中 `pred` 的参数类型是 `proj` 的返回类型，
  不是元素类型。如果搞错了 predicate 的签名，concept 约束会给出比裸模板更清晰的
  编译错误。
- projection 只能在 ranges 算法中使用。C++17 的 `std::sort` / `std::find`
  不接受 projection 参数，这是两者接口设计的根本差异之一。

## 常见坑

- **以为 projection 只能是成员指针**：projection 的 concept 约束是
  `std::invocable<Proj, element_type>`——任何 Callable 都可以。
- **把 projection 和 comparator 顺序搞错**：正确签名是
  `ranges::sort(range, comp, proj)`——comp 在前，proj 在后。
  记忆技巧：先写"判断标准"（comp/pred），再写"映射方式"（proj）。
- **对 minmax/min/max 期待返回迭代器**：`ranges::min` / `ranges::max` /
  `ranges::minmax` 返回元素的**值**（引用），不是迭代器；
  想要迭代器用 `min_element` / `max_element` / `minmax_element`。
- **用 C++17 `std::sort` 传 projection**：
  `std::sort(v.begin(), v.end(), comp, proj)` 在 C++17 中根本不存在这个重载。

## 提示

- 把 `ranges::sort(v, std::less{}, &Person::age)` 和
  `std::sort(v.begin(), v.end(), [](auto& a, auto& b){ return a.age < b.age; })`
  放在同一个文件里，通过 diff 直观感受 projection 带来的精简度。
- 检查 `minmax_result` 的成员：`auto [lo, hi] = ranges::minmax(v)` 中
  `lo` 对应 `.min`，`hi` 对应 `.max`；绑定顺序按成员声明顺序，不要搞反。
- 如果 projection 是 lambda，确保它是无状态（无捕获）或只读捕获的——
  带可变状态的 lambda 在 const 上下文下会遇到与 transform_view 类似的 const
  operator() 问题。

## 复盘问题

1. 有一个 `vector<pair<int, string>>`，用 `ranges::sort` 按 `second`（string）
   升序排列，projection 应该写什么？comp 应该写什么？
2. `ranges::find(people, "Alice", &Person::name)` 中，第二个参数 `"Alice"` 是
   与 `proj(*it)` 的结果做 `==` 比较的——如果 name 是 `std::string` 而传入的是
   `const char*`，这里的类型提升是隐式发生的，能解释原因吗？
3. `std::identity{}` 作为 projection 和完全不传 projection 在运行时行为上是否有差异？
   在编译期有差异吗（零开销）？
4. 为什么 C++17 风格算法无法添加 projection 参数——这是接口设计约束还是语言机制约束？

## 对应官方参考

- P0896R4：C++20 ranges 核心合入，包含 ranges 算法的 projection 参数设计
- cppreference: [std::ranges::sort](https://en.cppreference.com/w/cpp/algorithm/ranges/sort)
- cppreference: [std::ranges::min_max_result](https://en.cppreference.com/w/cpp/algorithm/ranges/min_max_result)
- cppreference: [Constrained algorithms 全列表](https://en.cppreference.com/w/cpp/algorithm/ranges)
