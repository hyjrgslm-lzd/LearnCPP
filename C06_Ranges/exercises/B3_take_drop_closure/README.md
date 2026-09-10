> 对应章节：../../03-模块B-基础适配器与管道.md § 练习 B-3：take / drop 家族与管道组合

## 目标

观察 `views::take` / `views::take_while` / `views::drop` / `views::drop_while`
四者对 `sized_range` / `common_range` / `iterator_concept` 的不同影响，以及管道语法
背后的 `range_adaptor_closure` 组合机制（P2387R3）。
这道题的重点是"四个 adaptor 的语义差异"和"管道是嵌套 view 而不是函数调用链"。

## 前置理解

- 做完练习 B-1 和 B-2，知道 `views::all` 的包装路径和 filter 的降级规则。
- `sized_range`：要求 `ranges::size(r)` 是 O(1)；`common_range`：begin / end 同类型。
- `range_adaptor_closure`（P2387R3）：绑定除 range 参数外所有参数的部分应用对象，
  支持 `operator|` 把自身应用到 range，也支持两个 closure 之间的 `|` 组合。

## 预计练习方向

属性对比表（填写观察结论时以此为目标）：

| adaptor | sized | common | iterator_concept | begin() const |
|---------|-------|--------|------------------|---------------|
| `take(n)` 对 random_access+sized | 保持 | 保持 | 透传 | 是 |
| `drop(n)` 对 random_access+sized | 保持 | 保持 | 透传 | 是 |
| `drop(n)` 对 bidirectional(list) | 保持 | 保持 | 透传（非 RA） | 是（O(n) 扫描） |
| `take_while(pred)` | 失去 | 失去（sentinel） | 透传 | 是 |
| `drop_while(pred)` | 失去 | 保持 | 透传 | 否（缓存写入） |

关键实验：
1. `views::iota(1) | views::take(10)` 将无界变有界，验证 `sized_range`。
2. 管道三种等价形式：直接管道 / 预组合 closure / 函数调用语法，用 `same_as` 验证
   三者产出完全相同的嵌套 view 类型。

## 进阶预计方向

**A. drop_while 的 begin() 缓存类比 filter**：第一次 begin() 调用线性扫描找到
第一个不满足 pred 的位置并缓存（`optional<iterator>` 写入），第二次直接返回缓存。
与 filter_view 相同：begin() 非 const，`const` 存储后无法迭代。
关键差异：drop_while 找到起点后 `++it` 直接推进（不再检查条件），而 filter 的每次
`++it` 都需扫描到下一个满足条件的元素——这是两者 iterator_concept 差异的根本原因
（drop_while 透传，filter 降级）。

**B. take_while 与 filter 的语义对比**：
- 管道 X：`iota(1,11) | transform(sq) | take_while([](int x){ return x < 50; })`
  产出 1, 4, 9, 16, 25, 36, 49（所有平方 < 50 的**前缀**，遇第一个不满足立即停止）。
- 管道 Y：`iota(1,11) | filter(is_odd) | transform(sq)`
  产出 1, 9, 25, 49, 81（所有奇数的平方，**全局**跳过式筛选）。
take_while 是前缀截断，filter 是全局跳过式筛选，两者根本不同。

## 验收点

- 能填写四个 adaptor 的属性对比表（sized / common / iterator_concept 是否保持，
  begin() 是否 const）。
- 能解释为什么 `drop` 对 random_access + sized range 是 O(1)，对 forward range 是 O(n)。
- 能解释 `drop_while` 需要缓存 begin() 的原因，以及与 `filter_view` 缓存的异同。
- 能演示 `views::iota(1) | views::take(10)` 从无界变有界，并指出 `unreachable_sentinel`
  被替换的位置。
- 能用 `static_assert(std::same_as<...>)` 证明管道语法和预组合语法产生相同类型。

## 观察点

- take 和 drop 都不降级 `iterator_concept`：它们只限制上界或偏移起点，不需要在
  递增时扫描条件，概念完全透传。
- `take_while` 的条件在 sentinel 的比较操作里（`end()` 是检查条件的 sentinel），
  不在递增里——因此 iterator_concept 透传，但 end() 类型不同于 begin()，结果非
  `common_range`。
- `drop_while` 的条件只在 begin() 初始化时执行一次；filter 的条件在每次 `++it`
  时执行。这是两者 iterator_concept 差异（drop_while 透传，filter 降级）的根本原因。
- 管道语法的本质：`r | a | b` 是标准左折叠——先算 `r | a` 构造一个 view，再算
  `view | b` 构造另一个 view。`range_adaptor_closure` 的 `operator|` 是普通运算符
  重载，没有任何"延迟整个表达式"的特殊机制。

## 常见坑

- **认为 take_while 能变成 sized_range**：take_while 的结束条件必须在运行时扫描
  才能知道，永远不是 `sized_range`。
- **认为 drop_while 没有 begin() 缓存**：drop_while 和 filter 一样有缓存，同样
  无法 const 迭代，这一点经常被忽视。
- **把 take_while 当 filter 用**：两者语义根本不同，仅在数据保证"满足条件的元素
  都在序列前部"时才等价。
- **认为 drop 对所有 range 都是 O(1)**：只有 random_access + sized range 才能
  O(1) 跳过；forward range 的 drop 在 begin() 时做 O(n) 扫描。
- **lambda 类型混用**：在验证三种管道等价形式时，若 `is_odd` / `sq` 写了多个
  lambda 字面量，即使源文本一致也是不同闭包类型，`same_as` 断言必然失败。
  务必把 lambda 提取为命名变量。

## 提示

- 把四个 adaptor 的属性做成横轴为 adaptor、纵轴为属性的表格来记忆，比逐一背诵
  更系统（参见上方"预计练习方向"中的对比表）。
- `drop_while` 的 begin() 非 const 测试与练习 B-2 的 filter 完全类似：故意用
  `const auto` 存管道再迭代，读编译错误。
- 管道类型的 `same_as` 验证需要 lambda 类型完全相同，务必把 lambda 提取为命名
  变量——lambda 类型不同本身也是一个有价值的观察点。
- 用 `if constexpr (std::ranges::sized_range<decltype(r)>)` 加运行时打印，
  直观展示属性表格中的 true/false 值。

## 复盘问题

1. 为什么 `take_view` 对 random_access + sized 底层是 `common_range`，但
   `take_while_view` 不是？两者 end() 实现的根本区别是什么？
2. `drop(n)` 和 `drop_while(pred)` 都"跳过前缀"，但 begin() 的实现路径不同：
   一个在构造 view 时确定起点，另一个在第一次 begin() 时确定。具体各是哪种？
3. 管道 `a | b`（其中 a 和 b 都是 `range_adaptor_closure`）产出的新对象是什么类型？
   为什么从类型层面这是合法的？（参考 P2387R3 的正式化）
4. 无界 `views::iota(1) | views::drop(5)` 是否可以安全迭代？
   对 forward 无界序列接 drop 会发生什么？

## 对应官方参考

- **P0896R4**：C++20 ranges 核心合入（take / drop / take_while / drop_while 的设计）
- **P1035R7**：C++20 ranges 算法化改造（take_while / drop_while / drop 等适配器正式加入）
- **P2387R3**：`std::ranges::range_adaptor_closure` 正式化（operator| 语义与 CRTP 基类）
- cppreference：[`std::ranges::take_view`](https://en.cppreference.com/w/cpp/ranges/take_view)
- cppreference：[`std::ranges::drop_view`](https://en.cppreference.com/w/cpp/ranges/drop_view)
- cppreference：[`std::ranges::take_while_view`](https://en.cppreference.com/w/cpp/ranges/take_while_view)
- cppreference：[`std::ranges::drop_while_view`](https://en.cppreference.com/w/cpp/ranges/drop_while_view)
- 01 心智模型中"range adaptor 与 range adaptor closure"章节
## 参考解析

预测：对 sized/random_access 底层，`take` / `drop` 常能保留 sized 和 random_access；对 list 底层只能保留 bidirectional；对无界 iota 截断后的 common/sized 状态由具体适配器决定。`take_while` 由谓词决定停止点，通常不能保持 sized。

当前程序检查 take/drop/drop_while/take_while 的输出，并验证预组合 closure 与直接管道类型一致。扩展时把观察项落实成实际 `static_assert` + `check`，完成标准是行为和类型都被观察到。
