> 对应章节：../../03-模块B-基础适配器与管道.md § 练习 B-2：filter / transform 与 iterator 降级

## 目标

观察管道中每一层 view 对 `iterator_concept` 的影响，建立"filter 必然降级，
transform 保持"的直觉，并理解 `filter_view::begin()` 非 const 的设计根源。
这道题的重点不是管道的输出值，而是管道每层类型系统里的迭代器概念变化。

## 前置理解

- `iterator_concept` 双轨（`iterator_concept` vs `iterator_category`）的存在——
  对于 proxy reference 迭代器两者可以不同（见 01 心智模型双轨章节）。
- `filter_view::begin()` 是非 const 成员函数，因为它需要缓存第一个满足条件的位置：
  结果存入 `optional<iterator>`，写入操作要求 `*this` 非 const。
- `views::all` 是所有 adaptor 的隐式规范化入口（见练习 B-1）。

## 预计练习方向

逐层构造并验证 iterator_concept：

| 层次 | 管道构造 | 预期 iterator_concept |
|------|----------|-----------------------|
| 0 | `vector<int>::iterator` | `random_access_iterator` |
| 1 | `views::all(v)` | `random_access_iterator`（透传） |
| 2 | `v \| views::filter(is_odd)` | `bidirectional_iterator`，非 random_access |
| 3 | `v \| views::filter(is_odd) \| views::transform(sq)` | `bidirectional_iterator`（transform 透传） |
| 4 | `v \| views::transform(sq)` | `random_access_iterator`（仅 transform） |

每层用 `static_assert` 验证。遍历层 3 输出结果（预期：1 9 25）。

注意：把 `is_odd` / `sq` 提取为**命名变量**，不同 lambda 表达式即使源文本一致
也是不同闭包类型，会在类型比较中产生混乱。

## 进阶预计方向

**A. iota 底层的降级路径**：把底层换成 `views::iota(1,6)`（random_access），
   观察 filter 接入后同样降为 bidirectional，transform 单独接入后保持 random_access。
   证明降级规则与底层无关，只取决于 adaptor 本身。

**B. const 管道 + filter 的编译失败**：用 `const auto pipe = v | views::filter(is_odd);`
   存管道，然后用 range-for 迭代，观察编译错误——`filter_view::begin()` 是非 const
   成员，缓存写入要求非 const 对象。C++23 的 `views::as_const`（P2278R4）解决的是
   "把可变元素包装为 const 元素"，不改变 `begin()` 的 const 约束。

**C. transform 的 const 可调用要求**：用带 `mutable` 修饰的 lambda 作为 transform
   的函数，先通过非 const 管道迭代（OK），再用 `const auto` 存管道迭代，观察编译
   失败——`const transform_view` 的 `begin()` 需要 `const F::operator()`，mutable
   lambda 不满足。解决方法：改用无捕获或只读捕获的 lambda。

## 验收点

- 能用 `static_assert` 分别确认 `filter_view` 的降级和 `transform_view` 的保持。
- 能解释 filter 必须降为 bidirectional 的原因：递增时需向前扫描，
  无法 O(1) 随机跳转。
- 能说明 `filter_view::begin()` 非 const 的原因：需把首个满足条件的迭代器位置
  写入内部 `optional<iterator>` 缓存。
- 能解释带可变捕获的 lambda 不能用于 const `transform_view` 迭代的原因。
- 能把观察连接到模块 A：工厂视图（如 `iota_view`）本身 const 可迭代，但接入
  `filter_view` 后 const 迭代能力被破坏。

## 观察点

- filter 降级是 **concept 语义决定**的，不是实现选择：`random_access_iterator`
  要求 `it + n` 是 O(1)，filter 迭代器做不到。
- begin() 缓存的必要性：第一次调用做线性扫描，结果存入 `optional<iterator>`；
  第二次调用直接返回缓存，避免重复扫描。缓存写入 = begin() 非 const。
- 管道里 filter 的位置决定全局上界：一旦有 filter，其后所有层的
  `iterator_concept` 上界都是 bidirectional，再多层 transform 也无法提升。
- `ranges::distance(r)` 对 `sized_range` 是 O(1)，对 `filter_view` 是 O(n)——
  另一类性能陷阱。

## 常见坑

- **认为 filter 后还是 random_access**：`v | views::filter(pred)` 的结果迭代器
  永远是 bidirectional，不因底层是 vector 而保持 random_access。
- **用 `const auto` 存含 filter 的管道再迭代**：C++20 下直接编译失败。
  C++23 的 `views::as_const` 不解决这个问题。
- **把 `iterator_category` 和 `iterator_concept` 混淆**：对于 proxy reference
  迭代器两者可以不同（见 01 心智模型双轨章节）；本题的 filter/transform 两者一致，
  但不要忘记双轨的存在。
- **忘记 filter_view 的缓存失效风险**：底层数据在第一次 begin() 后被修改
  （如 push_back 导致重新分配），缓存的迭代器失效，行为未定义。

## 提示

- 故意对 `const filter_view` 调用 begin()，读编译器报错里的 "requires non-const"
  字样，比看文档记忆更深。
- 画管道草图时在每层标注 `[random_access]`、`[bidirectional]`，视觉上强化降级位置。
- 把 `is_odd` / `sq` 提取为命名 lambda 变量，避免闭包类型混乱。

## 复盘问题

1. 把管道里 filter 和 transform 的顺序互换（先 transform 后 filter），
   迭代器降级结果有变化吗？
2. 为什么 C++20 标准不用 `mutable` 成员字段来缓存，而是让 begin() 非 const？
   （提示：view 的 O(1) copy 公理与 mutable 缓存的交互问题）
3. 在 const 上下文里消费含 filter 的管道，有哪几种合法方案？各自的代价是什么？
4. transform 的函数对象是否有 const 可调用的 concept 约束？
   在什么情况下触发编译失败？

## 对应官方参考

- **P0896R4**：C++20 ranges 核心合入（`filter_view` 与 `transform_view` 的设计）
- cppreference：[`std::ranges::filter_view`](https://en.cppreference.com/w/cpp/ranges/filter_view)
  （注意 begin() 的非 const 说明）
- cppreference：[`std::ranges::transform_view`](https://en.cppreference.com/w/cpp/ranges/transform_view)
  （注意 const-iterable 要求 F 的 const 可调用）
- 01 心智模型中"iterator_concept 双轨"章节
## 参考解析

预测：`filter` 的迭代器概念由底层和标准上界共同决定。vector 底层从 random_access 裁顶到 bidirectional；forward_list 底层保持 forward；input 底层保持 input。`transform` 通常透传底层迭代器概念，但引用类别取决于函数对象返回值。普通 vector 底层的 `filter_view` 仍不能 const 迭代；P3725R3 只放开 const input_range 特例。

当前程序同时验证 vector 管道、transform-first 管道、forward_list filter，以及可观察输出 `{1, 9, 25}`。扩展时应先写出底层 range concept，再推导 filter 裁顶，而不是背“filter 降级”一句话。
