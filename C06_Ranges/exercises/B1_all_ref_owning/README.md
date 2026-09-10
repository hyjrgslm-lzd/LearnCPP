> 对应章节：../../03-模块B-基础适配器与管道.md § 练习 B-1：all / ref_view / owning_view

## 目标

观察 `views::all(lvalue)` 和 `views::all(rvalue)` 分别得到什么 view 类型，建立对
`ref_view` / `owning_view` 包装决策的直觉，理解 `viewable_range` concept 的真实含义。
这道题的重点不是"用 all 做什么变换"，而是"all 在做什么包装决策"。

## 前置理解

- view 的三条语义公理：O(1) move、O(1) copy（若 copyable）、O(1) destroy。
- `borrowed_range` 的含义：迭代器有效性不依赖 range 对象的生命周期。
- `views::all` 是 CPO（定制化点对象），内部做三路条件分支：
  - 已是 view → 直接透传；
  - 左值 → 包成 `ref_view`；
  - 右值 → 包成 `owning_view`（P2415R2 引入）。

## 预计练习方向

1. **左值路径**：`views::all(vec)` → `ref_view<vector<int>>`；验证是 `borrowed_range`；
   调用 `vec.push_back(6)` 后通过 `rv` 观察到修改，证明借用语义。
2. **右值路径**：`views::all(std::vector<int>{10,20,30})` → `owning_view<vector<int>>`（P2415R2）；
   验证不是 `borrowed_range`；拥有数据，不会悬垂。
3. **透传路径**：`views::all(iota_view)` → 同类型直接透传；`same_as` 断言验证。
4. **viewable_range 概念**：分别验证左值引用与右值容器满足 `viewable_range`，
   理解两者的不同满足路径。

## 进阶预计方向

- 阅读 `viewable_range` concept 的条件分支，说明 P2415R2 之前为何
  `views::all(std::vector{1,2,3})` 非法（无 `owning_view`，右值非 `borrowed_range` 无法包成
  `ref_view`，否则立即悬垂）。
- 用 `std::string_view`（本身是 view + borrowed_range）验证透传；
  用右值 `std::string` 验证得到 `owning_view<string>`。
- 对比 `ref_view` 与 `std::span`：两者均是借用语义，但 `span` 是
  `contiguous_range`，`ref_view` 保持底层 range 的迭代器类别（如 `ref_view<list<int>>`
  不是 contiguous）。

## 验收点

- 能用 `static_assert` + `std::same_as` 区分左值路径（`ref_view`）和右值路径
  （`owning_view`）。
- 能说明 `ref_view` 是 `borrowed_range` 而 `owning_view` 不是，以及各自的悬垂风险
  在哪里。
- 能解释 P2415R2 解决了什么问题，以及"已是 view"为何直接透传。
- 能解释 `viewable_range` concept 的三条分支判断逻辑。

## 观察点

- `ref_view` 的核心状态是一个指向原容器的**指针**——O(1) move / copy / destroy 均来源于此，
  析构不释放任何数据。
- `owning_view` 把容器移入自身，是 move-only 的（保持 view 的 O(1) move 公理，
  不承诺 O(1) copy）。
- `views::all` 本身不是 view，它是 CPO；所有 range adaptor 在接受第一个 range 参数时
  都**隐式**调用它做规范化，对调用者透明。
- `vec | views::transform(f)` 底层第一层永远是 `ref_view<vector<int>>`（若 vec 是左值），
  即使没有显式写出 `views::all`。

## 常见坑

- **把 all 当身份转换**：`views::all(vec)` 返回 `ref_view`，类型不同于 `vector<int>&`，
  传给接受引用的函数会类型不匹配。
- **忽略 ref_view 的悬垂风险**：`auto r = views::all(local_vec); return r;`
  在函数返回后 `local_vec` 销毁，`r` 悬垂，编译器不报错。
- **认为 owning_view 是 borrowed_range**：`owning_view` 拥有数据，迭代器有效性依赖
  `owning_view` 对象本身的生命周期。

## 提示

- 用 `static_assert(std::same_as<decltype(expr), ExpectedType>)` 比 `is_same_v`
  更直接，失败时编译器报出实际类型，便于纠错。
- 想观察 `ref_view` 悬垂，故意让原容器超出作用域后再访问，用 AddressSanitizer 捕获；
  `owning_view` 版本不会出错。
- 用 `std::ranges::borrowed_range<T>` 这个 concept 做 `static_assert` 比看文档记忆更直接。

## 复盘问题

1. `ref_view` 和 `std::span` 都是借用语义，根本区别是什么？
   （提示：span 是 contiguous_range，ref_view 保持底层 range 的迭代器类别）
2. P2415R2 引入 `owning_view` 之前，如何安全地把临时容器传入管道？
3. 为什么 `owning_view` 不是 `borrowed_range`，但 `ref_view` 是 `borrowed_range`？
   迭代器有效性分别依赖什么对象的生命周期？
4. `views::all` 在管道里是显式写出来的还是被 adaptor 隐式调用的？
   它的存在对调用者透明吗？

## 对应官方参考

- **P0896R4**：C++20 ranges 核心合入（`ref_view`、`views::all`、`viewable_range` 的初始设计）
- **P2415R2**：`owning_view` 引入，view 语义澄清（"what is a view"）
- cppreference：[`std::ranges::ref_view`](https://en.cppreference.com/w/cpp/ranges/ref_view)
- cppreference：[`std::ranges::owning_view`](https://en.cppreference.com/w/cpp/ranges/owning_view)
- cppreference：[`std::views::all`](https://en.cppreference.com/w/cpp/ranges/all_view)
- cppreference：[`std::ranges::viewable_range`](https://en.cppreference.com/w/cpp/ranges/viewable_range)
## 参考解析

预测：`views::all(lvalue vector)` 产生 `ref_view`，`views::all(rvalue vector)` 产生 `owning_view`，已有 view（如 `iota_view`）会按值传递并保持 view 身份。`owning_view` 说明 view 可以拥有元素，但它仍不是“借用延寿”机制。

当前程序用 `static_assert` 区分 `ref_view` / `owning_view`，再用运行时修改原 vector 证明 `ref_view` 观察外部对象，用累加证明 `owning_view` 持有自己的元素。扩展时可比较 `span`、`string_view` 与 `vector`：前两者本身是轻量借用 view，后者需要 `all` 转换。
