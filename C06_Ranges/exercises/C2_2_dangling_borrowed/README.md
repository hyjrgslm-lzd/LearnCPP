> 对应章节：05-模块C2-算法·投影·范围边界 · 练习 C2-2：ranges::dangling 与 enable_borrowed_range 自定义

# C2-2：ranges::dangling 与 enable_borrowed_range 自定义

## 目标

观察 `ranges::find` / `ranges::copy` / `ranges::equal_range` 等算法返回的不是
"单个裸迭代器"，而是结构化的富返回类型或 `ranges::dangling`，并用
`static_assert` + `std::same_as` 在类型层面验证 `borrowed_range` 判定的完整逻辑。
动手实现一个自定义类型的 `enable_borrowed_range` opt-in，亲眼看到特化前后的差异。

## 前置理解

- 已在 `01-心智模型.md` 的"借用（borrowed）与悬垂（dangling）"段理解
  `ranges::dangling` 的基本概念：对右值非 `borrowed_range` 调用返回迭代器的算法，
  得到 `ranges::dangling`。
- 知道 `std::string_view` 是 `borrowed_range`（不拥有字符数据），
  而 `std::vector<int>` 不是（拥有元素）。
- 了解 `enable_borrowed_range` 是 opt-in 机制：标准库中少数类型默认特化为 `true`
  （`string_view`、`span`、`subrange`、`iota_view`、`empty_view` 等），
  其余类型默认是 `false`。

## 预计练习方向

1. **ranges::dangling 基础验证**：
   声明 `make_vec()` 返回右值 `vector<int>`，
   `auto it = ranges::find(make_vec(), 4)` 后，
   用 `static_assert` 验证 `it` 类型是 `ranges::dangling`。
   注释掉 `*it`，说明 `dangling` 没有 `operator*` 是编译期保护。
2. **string_view 是 borrowed_range**：
   `auto it_sv = ranges::find(std::string_view{"hello ranges"}, 'r')` 后，
   验证类型不是 `dangling`，且是 `contiguous_iterator`，打印偏移量（应为 6）。
3. **自定义 my_borrowed_span + enable_borrowed_range opt-in**：
   实现持有 `ptr + len` 的轻量 span，先验证默认不是 `borrowed_range`，
   再特化 `enable_borrowed_range = true`，
   用右值实例调用 `ranges::find` 验证返回类型是 `int*` 而非 `dangling`。
4. **borrowed_iterator_t / borrowed_subrange_t 类型别名验证**：
   验证 `borrowed_iterator_t<string_view>`、`borrowed_iterator_t<vector<int>>`、
   `borrowed_subrange_t<my_borrowed_span<int>>` 的展开结果。
5. **常见 borrowed view 一览**：
   用 `static_assert` 批量验证 `string_view`、`span`、`iota_view`（borrowed）
   和 `vector`、`string`、`owning_view`（非 borrowed）。

## 进阶预计方向

- `ranges::copy` 的 `in_out_result` 结构化绑定：
  `auto [in_end, out_end] = ranges::copy(src, dst.begin())`，
  验证 `in_end == src.end()`，`out_end == dst.end()`。
- `ranges::equal_range` 返回 `subrange<iterator, iterator>`：
  在已排序 `vector` 上查找等值范围，打印子范围并验证 `size() == 3`。
- 探索 `P2017R1` conditionally borrowed：`iota_view<int, unreachable_sentinel_t>`
  与 `iota_view<int, int>` 的 borrowed 属性是否相同？

## 验收点

- 能用 `static_assert(same_as<decltype(it), ranges::dangling>)` 在编译期验证
  右值 vector 场景。
- 能用 `static_assert` 验证 `string_view` 场景下返回的是真实的
  `string_view::iterator`。
- 能解释 `enable_borrowed_range` 是 opt-in 而非 opt-out：标准默认所有类型为
  `false`，用户自定义类型需要显式特化。
- 能用代码演示 `my_borrowed_span` 在特化前后，`ranges::find` 返回类型的变化。
- 能说出 `ranges::copy` 的返回类型是 `in_out_result`，并正确读取 `.in` 和 `.out`。

## 观察点

- `ranges::dangling` 是 empty 类型，没有任何成员函数（没有 `operator*`、
  没有 `operator->`）。任何试图"使用"它的操作都是编译期错误——把运行时悬垂迭代器
  的 UB 升级为编译期错误，这正是它作为"类型级安全守卫"的价值。
- `borrowed_range` 的判定发生在调用点的模板实例化期，不是运行时。
  `enable_borrowed_range` 是编译期 `constexpr bool`，ranges 算法通过 `if constexpr`
  或 concept 约束在编译期选择返回类型。
- `enable_borrowed_range` 的语义承诺：特化为 `true` 相当于承诺"这个类型的迭代器
  有效性不依赖对象本身的生命周期"。如果承诺是假的，通过 `ranges::find` 拿到的
  "真实迭代器"在对象销毁后变成悬垂——这是你撒谎的后果，不是标准库的 bug。
- 不要夸大"view 都是 borrowed"：只有极少数 view 默认是 `borrowed_range`。
  `transform_view`、`filter_view`、`take_view`、`owning_view` 等都**不是**
  `borrowed_range`。

## 常见坑

- **以为所有 view 都是 borrowed_range**：错误。只有 `string_view`、`span`、
  `subrange`、`iota_view`、`empty_view` 等少数类型默认特化为 `true`。
- **以为 `std::vector` 是 borrowed_range**：`vector` 拥有其元素的存储，
  当 vector 对象销毁时迭代器立即悬垂，所以它不是 `borrowed_range`。
- **忘记 dangling 没有 operator***：期待运行时崩溃告知问题——实际是编译期错误。
- **混淆 borrowed_range 和 view**：view 和 borrowed_range 是正交概念。
  `owning_view` 是 view 但不是 borrowed_range；`span` 既是 view 也是 borrowed_range。

## 提示

- 验证 `enable_borrowed_range` 特化前后的行为变化时，最直接的方法是把特化行
  注释掉，对比同一段代码在有/无特化时的编译结果——前者通过，后者失败。
- 用 `borrowed_iterator_t<R>` 和 `borrowed_subrange_t<R>` 作为返回类型声明，
  能在函数签名层面直接表达"返回类型依赖于 R 是否为 borrowed_range"的意图。

## 复盘问题

1. `ranges::find(string_view{"hello"}, 'e')` 对右值 `string_view` 能返回真实迭代器，
   但 `ranges::find(string{"hello"}, 'e')` 对右值 `string` 返回 `dangling`——
   两者的根本差异是什么？
2. 如果你有一个自定义类型既是 view 又应该是 borrowed_range，需要做什么操作让
   标准库算法认可这个事实？
3. `ranges::copy` 返回的 `in_out_result::in` 和 `::out` 分别指向什么位置？
   为什么需要返回这两个迭代器而不是只返回输出迭代器？
4. `enable_borrowed_range` 的特化写在哪个命名空间？
   可以在用户代码里特化标准库类型吗？

## 对应官方参考

- P0896R4：C++20 ranges 基础，包含 `ranges::dangling`、`borrowed_range` concept
- P1739R4：borrowed_range 细化（哪些标准类型应该是 borrowed_range）
- P2017R1：conditionally borrowed（某些 view 在模板参数满足条件时才是 borrowed_range）
- P1207R4：iota_view borrowed 属性
- cppreference: [std::ranges::dangling](https://en.cppreference.com/w/cpp/ranges/dangling)
- cppreference: [std::ranges::borrowed_range](https://en.cppreference.com/w/cpp/ranges/borrowed_range)
- cppreference: [std::ranges::enable_borrowed_range](https://en.cppreference.com/w/cpp/ranges/borrowed_range)
## 参考解析

预测：右值非 borrowed range 调返回迭代器的算法会得到 `ranges::dangling`；`string_view`、`span`、`iota_view` 是 borrowed，因为迭代器指向外部或值语义位置，不依赖 view 对象本身。borrowed 不负责持有拥有者生命周期。

当前程序验证右值 vector 的 dangling、string_view 的真实迭代器、自定义 span 包装通过 `enable_borrowed_range` 获得 borrowed，以及 `borrowed_iterator_t` / `borrowed_subrange_t` 的类型结果。扩展时不要把 borrowed 当 GC 或 shared ownership。
