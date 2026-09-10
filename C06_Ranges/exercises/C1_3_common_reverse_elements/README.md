> 对应章节：04-模块C1-结构适配器 · 练习 C1-3：common / reverse / elements / keys / values

# C1-3：common / reverse / elements / keys / values

## 目标

一次性把"结构投影"这一组适配器过完：`views::common`、`views::reverse`、
`views::elements<N>`、`views::keys`、`views::values`。
建立心智词汇，知道每个适配器的适用场景、对 iterator_concept 的影响，
以及可能触发悬垂的右值 borrowed 问题。

## 前置理解

- 理解 `common_range` 概念：begin/end 类型相同的 range 才能直接传给 C++17 风格算法。
- 知道 `views::reverse` 需要双向迭代器。
- 理解 `borrowed_range`：对右值非 `borrowed_range` 调用返回迭代器的算法得到
  `ranges::dangling`。

## 预计练习方向

1. **common**：构造 `iota(1) | take(10)`，验证它不是 `common_range`，
   用 `views::common` 包装后传给 `std::accumulate`，确认结果 55。
   注释说明 C++20 ranges 算法无需 `common_range`，只有 C++17 接口才需要。
2. **reverse**：对 `vector` 取 `reverse`，打印 `5 4 3 2 1`。
   用 `static_assert` 验证 `reverse_view` 迭代器是 `random_access_iterator`。
   用 `static_assert` 验证 `reverse_view` 是 `common_range`（begin/end 同类型）。
   注释掉对 `forward_list` 取 `reverse`，确认 concept 会阻止编译。
3. **elements / keys / values**：对 `map<string,int>` 用 `views::keys` 和
   `views::values` 分别迭代。用 `static_assert` 验证
   `views::keys == views::elements<0>`（同一类型）。
   对 `vector<tuple<int,string,double>>` 用 `views::elements<0>` 和 `elements<1>`。
4. **borrowed + dangling**：声明返回右值 `map` 的函数，调用
   `ranges::find(get_map() | views::values, 2)`，
   用 `static_assert` 验证返回类型是 `ranges::dangling`。

## 进阶预计方向

- 验证 `common_view` 对已是 `common_range` 的输入是 no-op：
  `vector<int>` 套 `views::common` 后 begin/end 类型不变。
- 自定义一个满足 tuple 协议（`tuple_size`、`tuple_element`、`get<N>`）的类型，
  验证它可以被 `views::elements<0>` 投影。
- 理解 `views::elements<N>` 依赖 ADL 查找 `get<N>` 的机制，
  不仅限于 `std::tuple`——`std::pair`、`std::array` 以及满足 tuple 协议的自定义类型均可。

## 验收点

- 能用 `views::common` 把非 `common_range`（如 `iota | take`）包装成
  C++17 算法可用的形式，并证明 begin/end 类型现在相同。
- 能用 `static_assert` 证明 `forward_list` 不能被 `views::reverse`。
- 能写出 `views::keys` / `views::values` / `views::elements<N>` 的基本用法，
  并说出它们的等价关系。
- 能解释对右值 `map` 取 `views::values` 后算法返回 `ranges::dangling` 的原因。
- 能说出 `reverse_view` 的 iterator_concept 如何从底层 range 继承。

## 观察点

- `views::common` 的典型使用场景是"桥接 C++20 view 管道和 C++17 算法接口"。
  C++20 的 ranges 算法（`ranges::sort`、`ranges::find` 等）直接接受 iter/sentinel
  不同类型的 range，不需要 common；C++17 接口（`std::accumulate` 等）才需要。
- `views::reverse` 的约束是设计上必然：双向迭代器才能从尾部向头部推进。
  `forward_iterator` 只能单向，无法实现"从末尾开始前推"的语义。
  concept 约束在编译期就会阻拦不合法使用。
- `views::elements<N>` 依赖 `get<N>` 的 ADL 查找，这是 C++ tuple-like 协议。
  只要类型满足 `tuple_size`、`tuple_element`、`get<N>`，就可以被 `elements` 投影。
- `views::values` 作用到右值 `map` 上的悬垂风险与"右值非 `borrowed_range`
  传给返回迭代器算法"是同一个问题。`values_view` 不改变底层 range 的 borrowed 属性。

## 常见坑

- 对单向迭代器（`forward_list`、`iota` 无界序列、`filter_view` 等）用
  `views::reverse`，期待编译通过：concept 在调用点就报错，不会等到运行时。
- 对非 tuple-like 的元素类型用 `views::elements` / `views::keys` / `views::values`：
  如果元素类型不满足 tuple 协议，编译错误会比较冗长，
  要认出"没找到 get<N>"这类信息。
- 期待 `views::values` 对右值 `map` 返回的迭代器在 `map` 销毁后仍有效：
  `map` 不是 `borrowed_range`，对右值 `map` 做任何返回迭代器的操作都会触发
  `ranges::dangling` 保护。
- 在已经是 `common_range` 的 view 上再套 `views::common`，以为会有额外开销：
  标准规定 `common_view` 对 `common_range` 是 no-op，直接透传。
- 把 `views::keys` 当成返回所有 key 的新 `vector`：它是惰性 view，
  不会提前分配内存，消费时才按需读取 key。

## 提示

- 测试 `views::common` 时，先打出 `r.begin()` 和 `r.end()` 的类型，
  然后包装一层 `views::common` 再打印，直观看到类型变化。
- 测试 `views::reverse` 时，优先用 `static_assert`，把
  `static_assert(std::random_access_iterator<decltype(rv.begin())>)` 写出来。
- `views::elements<N>` 的 N 必须是编译期常量，不能是运行时变量，
  这是 tuple 协议的基本约束。
- 验证 `borrowed_range` 时，先用 `static_assert(!borrowed_range<map<...>>)`
  确认底层类型，再验证 `values_view` 是否继承了这个属性。

## 复盘问题

1. `views::common` 解决的是"C++17 接口要求 begin/end 同类型"的问题，
   而 C++20 ranges 算法已解除这个限制。未来哪些情况下仍然需要 `views::common`？
2. `views::reverse` 要求 `bidirectional_range`，能否放宽为 `forward_range`？
   放宽后会产生什么问题？
3. `views::values` 作用于左值 `map` 和右值 `map` 时，`values_view` 的 borrowed
   状态有什么不同？为什么右值 `map` 的 `views::values` 给 `ranges::find` 会触发
   `dangling`？
4. 如果你有一个自定义类型想让它支持 `views::elements<2>`，需要实现哪些接口？
5. 这一组适配器里，哪些会改变迭代器概念（降级或保持），哪些对迭代器概念没有影响？

## 对应官方参考

- P0896R4：C++20 核心合入，包含 `common_view`、`reverse_view`、`elements_view`
- cppreference: [std::ranges::common_view](https://en.cppreference.com/w/cpp/ranges/common_view)
- cppreference: [std::ranges::reverse_view](https://en.cppreference.com/w/cpp/ranges/reverse_view)
- cppreference: [std::ranges::elements_view](https://en.cppreference.com/w/cpp/ranges/elements_view)
- cppreference: [std::ranges::keys_view / values_view](https://en.cppreference.com/w/cpp/ranges/keys_view)
## 参考解析

预测：`common` 把 iter/sentinel 异型 range 包成 common_range；`reverse` 要求 bidirectional，且 iterator_concept 跟随底层；`elements` / `keys` / `values` 只投影 tuple-like 元素，不改变底层 borrowed 语义。

当前程序验证 common 化、vector reverse 保留 random_access、forward_list 不能 reverse、keys/values 输出，以及右值 map 参与返回迭代器算法时得到 dangling。扩展问题答案：右值 map 的元素生命周期随 map 销毁，borrowed_range 不能凭 values_view 自动获得。
