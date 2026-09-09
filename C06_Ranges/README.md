# C06：Ranges、视图与惰性求值

## 这套文档要解决什么问题

这不是一套"背 view 名字"的笔记，而是一套"通过亲手编码理解 ranges/views 设计"的练习包。

C++20 的 `<ranges>` 引入的不只是"方便的管道语法"，而是一套以概念（concept）为核心的迭代模型：range、view、sentinel、CPO、range adaptor closure、borrowed_range——每一个对象背后都有明确的语义公理和设计取舍。只有理解这些，你才能真正在复杂场景里正确使用 ranges，而不是靠猜测。

与本系列另一份文档的关系一句话总结：

> **`C10_Execution` 管何时执行，`C06_Ranges` 管如何取值。**
>
> stdexec 描述异步工作图的组合与调度；ranges 描述惰性值序列的产生与变换。两套抽象正交互补，合在一起才构成现代 C++ 的"数据从哪里来 + 工作在哪里跑"的完整模型。

## 你会得到什么

这套练习包分为两个阶段：

### 第一阶段：使用层——概念理解与管道组合

- 1 份心智模型总说明。
- 4 个模块（A/B/C1/C2）+ 1 个高阶模块（D），共约 15 道练习题。
- 1 个结课项目 + 1 条源码阅读路线。
- 每题统一的复盘框架。

### 第二阶段：实现层——自定义 view 与适配器开发

- 4 个模块（E/F/G/H），共 12 道练习题。
- 2 个结课项目（实现级源码对照 + mini view adaptor 子集实现）。
- 覆盖 view_interface、range adaptor closure 的 `operator|` 实现、iterator_concept 双轨、sentinel 实现、borrowed_range 标记、ranges::to 实现等核心技术。

### 总计

- **约 27 道练习题 + 4 个结课项目**

## 阅读顺序

### 第一阶段

1. `01-心智模型.md`
2. `02-模块A-视图工厂与惰性.md`
3. `03-模块B-基础适配器与管道.md`
4. `04-模块C1-结构适配器.md`
5. `05-模块C2-算法·投影·范围边界.md`
6. `06-模块D-C++23高阶视图与协程桥.md`
7. `07-结课项目1-数据管道与源码阅读.md`（第一阶段结课）

### 第二阶段

8. `08-模块E-CPO与niebloid.md`
9. `09-模块F-概念精化与迭代器分类.md`
10. `10-模块G-自行实现视图.md`
11. `11-模块H-高级实现模式.md`
12. `12-结课项目2-实现级源码阅读.md`（第二阶段结课）

## 统一技术基线

本练习包默认你已经具备本地编码条件，因此这里不写安装和工程搭建，只固定练习边界。

- **语言最低基线**：`C++20`（`-std=c++20`）
- **推荐编译器版本**：GCC 13+、Clang 17+、MSVC 19.38+（VS 2022 17.8+）
- **C++23 特性**：凡依赖 C++23 的示例，代码顶部注释标注 `// C++23: -std=c++23`；包括 `views::zip`、`views::chunk`、`views::slide`、`views::stride`、`views::cartesian_product`、`views::repeat`、`ranges::to`、`views::join_with` 等
- **常用头文件**：`<ranges>`、`<algorithm>`、`<iterator>`、`<functional>`、`<span>`、`<string_view>`
- **排除范围**：`<regex>`、`<locale>`、`<charconv>` 细节、`std::format` 格式化细节、GPU 并行、`nvhpc` 编译器特性
- **线程要求**：本系列不涉及并行执行，不写 `std::thread`；并行场景由 `C10_Execution` 覆盖

## 两个阶段的定位差异

### 第一阶段：你学的是"怎么用"

- 理解 range、view、iterator、sentinel、range adaptor closure 这六个核心对象
- 掌握 `views::transform`、`views::filter`、`views::take`、`views::drop`、`views::join`、`views::split` 等基础适配器
- 理解惰性求值、borrowed_range、dangling 返回值、iterator_concept 双轨
- 掌握 `ranges::sort`、`ranges::find`、`ranges::copy` 等带 projection 的约束算法
- 能手写最小 range 消费器（for-range 循环、`ranges::to`、算法调用）
- 能阅读标准库示例并理解 view 的组合关系

### 第二阶段：你学的是"怎么做"

- 理解 CPO / niebloid 这套定制化机制及其演进（`ranges::begin` 为什么不是函数模板）
- 理解 `view_interface` 如何通过 CRTP 注入默认成员
- 能实现 range adaptor closure（`operator|` 的部分应用对象）
- 能实现自定义 sentinel 与 `unreachable_sentinel_t` 类比
- 能实现支持 `iterator_concept` 双轨的自定义 iterator
- 能实现带 borrowed_range 标记的轻量 view
- 能实现简化版 `ranges::to`
- 能阅读 libstdc++ / libc++ / MSVC STL 的 view 实现并指出实现模式

## 你要始终记住的定位

### 1. ranges 不是"方便语法的算法包装"

很多人看到 `v | views::filter(pred) | views::transform(f)` 只觉得"管道比嵌套调用好看"。这远远不够。ranges 真正的设计中心是：

- 以 concept 约束迭代器层级（`input_iterator`、`forward_iterator`、`random_access_iterator`……）
- 分离 iterator 与 sentinel（`begin()` 和 `end()` 可以是不同类型）
- 明确区分"借用"与"拥有"（borrowed_range 的设计意图）
- 让 view 的 O(1) 语义成为公理，而不只是惯例

### 2. view 的语义公理不是性能提示

view 的 O(1) move、O(1) copy（若可拷贝）、O(1) destroy 是语义承诺，不是优化目标。违反这三条公理的类型不能满足 `std::ranges::view` concept，即使你强行标记也会导致错误的优化假设。

### 3. 这套文档不追求"最短可运行代码"

它追求的是：

- 你能把值变换路径拆成 view 依赖图
- 你能说清管道里每一层的迭代器概念
- 你能说清哪里是 borrowed，哪里可能产生 dangling
- 你能指出真正开始迭代的代码行
- （第二阶段）你能解释框架的实现选择，并能实现其中的关键子集

## 每题统一交付物

每完成一道题，至少留下四样东西：

1. 一份可运行代码。
2. 一张管道草图（每层 view 名字 + 迭代器概念标注）。
3. 一段 5 到 10 行的观察记录。
4. 一段复盘结论：这一题到底让你理解了什么设计点。

## 每题统一模板

所有练习题都按同一模板组织，但阶段一和阶段二略有差别——因为两阶段训练目标不同：

**阶段一（模块 A–D + 07，使用层）** 每题含：
- **目标** / **前置理解** / **预计练习方向**（本轮占位） / **进阶预计方向**（本轮占位） / **验收点** / **观察点** / **常见坑** / **提示** / **复盘问题** / **对应官方参考**

**阶段二（模块 E–H + 12，实现层）** 每题含：
- **目标** / **前置理解** / **必做任务**（描述实现路径而非留白占位） / **进阶任务** / **验收点** / **观察点** / **常见坑** / **复盘问题** / **对应官方参考**

这一差异是刻意的：阶段一偏观察与组合直觉，题目用占位"方向"保留探索空间；阶段二偏实现，题目直接描述该写什么类型、该满足哪个 concept，不留白。

你做题时也尽量按这个模板留笔记。这样你回看时，会非常容易发现自己到底卡在"概念没懂"，还是"API 没用熟"，还是"实现模式没理解"。

## 建议节奏

### 方案 A：第一阶段 6-8 天

- 第 1 天：`01-心智模型.md` + 模块 A 第一题
- 第 2 天：模块 A 剩余 + 模块 B 第一题
- 第 3 天：模块 B 第二、三题
- 第 4 天：模块 C1 全部
- 第 5 天：模块 C2 全部
- 第 6 天：模块 D 前两题
- 第 7 天：模块 D 后两题
- 第 8 天：结课项目

### 方案 B：第二阶段 8-10 天

- 第 1-2 天：模块 E（CPO / niebloid / ADL 隔离）
- 第 3-4 天：模块 F（`view_interface` 与 iterator 实现）
- 第 5-6 天：模块 G（range adaptor closure 实现）
- 第 7-8 天：模块 H（高级实现模式：borrowed 标记 / sentinel / `ranges::to`）
- 第 9-10 天：结课项目

### 方案 C：慢练，全程 20 天

- 每天只做 1-2 题
- 每两题安排一次 cppreference 回看
- 结课项目各预留 2 天

## 统一判定标准

如果你做完一道题，只是"代码跑了"，那还不够。至少再检查下面四件事：

- 你能指出管道里每层 view 的迭代器概念（`input_iterator` / `forward_iterator` / `bidirectional_iterator` / `random_access_iterator`）。
- 你能指出这条管道是否 borrowed，以及 dangling 在哪里可能出现。
- 你能指出真正触发迭代的代码行（for-range、`ranges::to`、算法调用，还是其他消费端）。
- 你能指出这道题里用了哪些 projection，以及不用 projection 的等价写法是什么。

第二阶段额外检查：

- 你能指出这道题涉及了哪种 C++ 实现技术（CPO / `view_interface` CRTP / range adaptor closure / sentinel / borrowed 标记等）。
- 你能指出这种技术解决了什么问题，以及它的替代方案是什么。

## 术语速查表

| 术语 | 你在练习里会看到什么 | 应该问自己什么 |
|------|----------------------|----------------|
| view | `views::transform(v, f)` 的结果、`string_view`、`span` | 它的 move/copy/destroy 是否都是 O(1)？它拥有数据还是借用数据？ |
| viewable_range | `views::all(r)` 能接受的参数类型 | 这个 range 可以被安全地转换成 view 吗？ |
| borrowed_range | `std::string_view`、`std::span`、裸指针范围 | 算法对这个 range 返回的迭代器，能不能安全使用到 range 销毁之后？ |
| sized_range | `std::vector`、`std::array`、`views::iota(0,10)` | `ranges::size(r)` 是 O(1) 吗？还是需要遍历才能知道大小？ |
| common_range | `begin()` 和 `end()` 类型相同的 range | 这个 range 能直接传给 C++17 风格算法（要求 begin/end 同类型）吗？ |
| sentinel | `views::take_while` 的 end 类型、`std::unreachable_sentinel` | 它代表什么终止条件？它和 iterator 是同一类型吗？ |
| CPO | `ranges::begin`、`ranges::end`、`ranges::size` | 这个"函数"为什么是函数对象而不是函数模板？ADL 怎么被隔离的？ |
| niebloid | `ranges::sort`、`ranges::find`、`ranges::transform` | 为什么它不被 ADL 找到？为什么可以当作函数对象传递？ |
| range_adaptor_closure | `views::transform(f)`（一个参数，等待 range） | 这是部分应用对象，还是已经绑定了完整参数的 view？`\|` 是怎么被重载的？ |
| projection | `ranges::sort(v, less{}, &Person::age)` 的第三个参数 | 投影在哪个位置被应用？默认是 `std::identity`，改掉它能省掉多少 lambda？ |
| dangling | `ranges::find(get_vec(), 42)` 对右值非 borrowed_range 的返回值 | 为什么返回 `ranges::dangling` 而不是迭代器？用了它会怎样？ |
| iterator_concept vs iterator_category | `zip_view` 的 iterator 类型有 `iterator_concept = random_access_iterator_tag` 但 `iterator_category = input_iterator_tag` | concept 描述 C++20 迭代器能力；category 兼容 C++17 算法期望的能力。proxy reference 导致两者不同。 |

## 统一编码约束

- 每题先写最小可观察版本，再做进阶任务。
- 优先记录每层 view 的迭代器概念、是否 borrowed、是否 sized。
- 不要过早追求通用库封装，先把 view 依赖图画清楚。
- 不要把 `views::filter` 当成自由变换的银弹：它会把迭代器降为 `bidirectional_iterator`（即使输入是 random access），并且在 C++20 里不能 const 迭代。
- 不要假设管道是"从左到右立即执行"；管道在被 for-range 或 `ranges::to` 之前不会真正迭代。

## 一个非常重要的现实提醒

### `filter_view` 的 const 迭代问题

C++20 的 `filter_view` 不支持 const 迭代：它的 `begin()` 需要缓存（cache）第一个满足条件的位置，因此 `begin()` 是非 const 成员函数。这意味着你无法对 `const filter_view` 对象调用 `begin()`。注意：P2278R4 引入的 `views::as_const` 与 `basic_const_iterator` 解决的是"把可变元素视图包装成 const 元素视图"的问题，并**不会**改变 `filter_view` 的 begin 缓存约束——C++23 的 `const filter_view` 依然无法迭代。

实际影响：如果你把一个含 `filter_view` 的管道存进 `const` 变量，编译器会报错。解决方案是不要 const 存管道，或者在 const 上下文里手动调用消费操作。

### `split_view` 的 C++20 vs C++23 语义变化

C++20 的 `views::split` 产出的子范围（subrange）不是 `forward_range`——子范围的 begin/end 类型不同，并且子范围本身只满足 `forward_range` 的部分约束，不能直接构造 `std::string`。C++23 的 P2210R2 重做了 `views::split`：原本 C++20 的惰性语义被命名为 `views::lazy_split` 保留，而 `views::split` 被重新定义为一个更贴近字符串切分直觉的版本——当输入是 `contiguous_range` 时子范围就是 `string_view` 兼容的。split 的 C++23 改动与 P2387R3（range_adaptor_closure 正式化）无关。

如果你在 C++20 下发现 `views::split` 不如预期，这不是你的错，这是标准有意为之的取舍。

### 编译器 C++23 ranges 支持程度不同

`views::zip`（P2321R2）、`views::chunk`、`views::slide`（P2442R1）、`views::stride`（P1899R3）、`views::cartesian_product`（P2374R4）、`views::repeat`（P2474R2）、`ranges::to`（P1206R7）等 C++23 新增视图，在 GCC 13、Clang 16/17、MSVC 19.38 上的支持情况各有差异。以你本地编译器通过为准，设计意图不变——如果某个 C++23 view 在你本地编译器不可用，优先理解它的语义，手写一个临时替代。

## 推荐做题方法

每题都按下面的顺序推进：

1. 先用一句话写出你认为这题在训练什么。
2. 先画 view 依赖图，标出每层的迭代器概念和 borrowed 状态，再落代码。
3. 先做基础任务，不要一开始就追进阶。
4. 跑通后，不马上进入下一题，先回答复盘问题。
5. 每做完一个模块，回去重读一次 `01-心智模型.md`。

## 做完整套之后你应该达到什么水平

### 第一阶段完成后

- 解释为什么 view 不是"轻量容器"，而是有语义公理约束的迭代适配器。
- 解释为什么 iterator 和 sentinel 分离之后，终止条件可以用完全不同的类型表达。
- 解释为什么 `filter_view` 会把迭代器概念降级，以及这对管道后续操作的影响。
- 解释 borrowed_range 的设计意图，以及算法为什么对非 borrowed 右值返回 `ranges::dangling`。
- 解释 projection 相比手写 lambda 的优势，以及它在算法内部的应用位置。
- 看懂一段 view 管道，说出每层的迭代器概念和 borrowed 状态。

### 第二阶段完成后

- 解释 CPO / niebloid 的设计意图和 ADL 隔离机制。
- 实现一个自定义 view（继承 `view_interface`，实现 `begin()` / `end()`，正确标记 borrowed_range）。
- 实现一个 range adaptor closure（接受一个参数，返回可被 `|` 左折叠的对象）。
- 实现一个支持 `iterator_concept` 双轨的 proxy-reference iterator。
- 实现简化版 `ranges::to<std::vector>`。
- 阅读 libstdc++ / libc++ 的 `transform_view` 实现并指出实现模式。

## 参考资料入口

做题过程中，建议反复对照下面这些资料的"概念定位"，而不是一上来通读全文：

### 核心提案

| 提案编号 | 内容摘要 |
|----------|----------|
| P0896R4 | C++20 核心合入：ranges 基础（range、view、iterator concepts、adaptor 框架） |
| P2214R2 | C++23 ranges 计划（`views::zip`、`views::chunk`、`views::slide` 等的设计背景） |
| P2387R3 | `std::ranges::range_adaptor_closure`：用户自定义 adaptor 的 `operator\|` 正式化 CRTP 基类 |
| P2502R2 | `std::generator`：C++23 协程与 ranges 的桥接 |
| P1206R7 | `ranges::to`：将 range 转换为容器的统一接口 |
| P2210R2 | C++23 `views::split` 语义改进（子范围可构造 string） |
| P2278R4 | `views::as_const`：为 non-const view 提供 const 视图包装 |
| P1035R7 | C++20 ranges 对标准算法的概念化改造 |
| P1207R4 | `ranges::iota_view` 的 borrowed_range 标记设计 |
| P2321 | C++23 `views::zip` 和 `views::zip_transform` |
| P2442 | C++23 `views::chunk`、`views::chunk_by`、`views::slide` |
| P1899 | C++23 `views::stride` |
| P2374 | C++23 `views::cartesian_product` |
| P2474R2 | C++23 `views::repeat`（同时修订 `cartesian_product` 空积等边缘情况） |
| P2446R2 | C++23 `views::as_rvalue` |
| P2441R2 | C++23 `views::join_with` |
| P2781R5 | C++23 `std::from_range_t` 容器构造标签 |
| P1252R2 / P1456R1 / P1614R2 / P1739R4 | C++20 ranges 清理与细化（设计清理、move-only view、spaceship、borrowed 细化） |
| P2325R3 / P2415R2 | C++23 fix-forward（view 不必默认构造、`owning_view` 与 view 语义澄清） |
| P2164 | C++26 `views::enumerate` |
| P2542 | C++26 `views::concat` |
| P2728 | C++26 Unicode 视图（`views::as_utf8/16/32`） |

### cppreference 入口

- [`std::ranges` — 顶级概念与工具](https://en.cppreference.com/w/cpp/ranges)
- [`std::ranges::view` concept](https://en.cppreference.com/w/cpp/ranges/view)
- [`std::ranges::borrowed_range` concept](https://en.cppreference.com/w/cpp/ranges/borrowed_range)
- [`std::ranges::dangling`](https://en.cppreference.com/w/cpp/ranges/dangling)
- [`std::ranges::view_interface`](https://en.cppreference.com/w/cpp/ranges/view_interface)
- [Range adaptors 全列表](https://en.cppreference.com/w/cpp/ranges#Range_adaptors)
- [Constrained algorithms 全列表](https://en.cppreference.com/w/cpp/algorithm/ranges)

## 最后一句提醒

不要把这套练习当成"我要赶快会写多少个管道"。

把它当成两层训练：第一层是使用层训练——你在学习一种把值序列的产生、变换、过滤、分组、消费全部用概念约束的惰性组合方式表达出来的设计模型。第二层是实现层训练——你在学习用什么 C++ 技术来构建这种模型。

两层都练透，你对 C++ 数据变换的理解就不再停留在"能用"，而是到达"能设计"。

## C02 引用与借用先修

[表达式与引用](../C02_Objects_Lifetime_Ownership/chapters/02-expressions-and-references.md)和[生命周期与借用](../C02_Objects_Lifetime_Ownership/chapters/03-lifetime-and-borrowing.md)为 view、iterator 与惰性访问提供先修。borrowed_range 讨论迭代器是否依赖 range 对象本身，不会自动延长底层 owner 的生命期；具体视图、迭代器与失效契约继续由本课主讲。

## C03 状态与类型擦除衔接

[C03 optional](../C03_Type_Modeling_Interface_Design/chapters/03-optional-and-empty-state.md)、[variant](../C03_Type_Modeling_Interface_Design/chapters/04-variant-and-state-space.md)及[类型擦除](../C03_Type_Modeling_Interface_Design/chapters/11-type-erasure.md)补齐缓存、判别状态和any_view所需基础。C++26 optional的0/1范围含义由C03引入，具体range协议在本课承接；擦除保留哪些迭代能力、引用和失效保证需要另行规定，不能自动推出稳定ABI。
