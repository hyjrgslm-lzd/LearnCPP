> 对应章节：../../08-模块E-CPO与niebloid.md 练习 E-3：ranges CPO 与 stdexec tag_invoke 对照

## 目标

横向对照 ranges CPO（C++20，P0896R4）和 stdexec `tag_invoke`（P1895R0/P2300）两套定制机制的设计演进，理解各自的适用场景和权衡。通过给同一个类型同时注册两套定制的练习，感受它们在"如何扩展"上的具体差异。

本练习给出 ranges 侧的完整实现；stdexec 侧仅给出最小桩和注释说明（不依赖 stdexec 库，stdlib only）。

## 前置理解

- **ranges CPO 的定制模式**（E-1 已实现过）：三阶查找，成员函数优先，ADL 自由函数 fallback，每个 CPO 各自独立定义查找逻辑。用户接入方式：给类型加成员 `begin()` 或在类型所在命名空间加 ADL `begin(r)`。
- **`tag_invoke` 的定制模式**（来自 P1895R0/P2300，不是 C++ 标准）：所有定制点共享同一个 ADL 函数名 `tag_invoke`，通过第一个参数（标签类型 tag）区分不同定制点。用户通过 `friend tag_invoke(connect_t, MyType, ...)` 在类体内声明 friend 函数接入。分发链：`cpo(args)` → `tag_invoke(cpo_tag, args)` → 用户的 `friend` 实现。
- **设计演进背景**：ranges 先于 stdexec（C++20 vs P1895），以"每个 CPO 独立管理 ADL"方式实现。stdexec 面对几十个定制点，发现独立 ADL 仍有潜在冲突，进一步收窄为"1 个 ADL 名字 + N 个 tag 类型"。两者都在防御 ADL 污染，抽象层次不同。

## 必做任务

1. **实现 `my_exec::connect_t` tag struct 和 `connect` CPO 本体**：`inline constexpr connect_t connect{}`，在 `operator()` 里通过 ADL 调用 `tag_invoke(*this, s)`。理解这是 tag_invoke 侧的 CPO 入口。

2. **实现 `MyContainer` 的 ranges 侧定制**：提供 `begin()` / `end()` / `size()` 成员函数，满足 `std::ranges::range` 和 `std::ranges::sized_range`。

3. **实现 `MyContainer` 的 tag_invoke 侧定制**：提供 `friend int tag_invoke(my_exec::connect_t, MyContainer&)` 返回第一个元素，演示单一 ADL 名字的接入方式。

4. **用 `static_assert` 验证四个 concept**：`std::ranges::range<MyContainer>`、`std::ranges::sized_range<MyContainer>`、`std::contiguous_iterator<int*>`、`std::is_object_v<decltype(my_exec::connect)>`。

5. **在 `demo_dual_customization` 里验证两条路径都工作**：通过 `std::ranges::begin(mc)` 走 ranges CPO 路径，通过 `my_exec::connect(mc)` 走 tag_invoke 路径，打印验证结果。

## 进阶任务

- **增加第二个 tag**：新建 `struct get_size_t {}; inline constexpr get_size_t get_size{};`，在 `MyContainer` 里加对应 `friend tag_invoke`。演示"新增定制点只需新建 tag"的扩展优势——与 ranges CPO 每个独立实现三阶查找的代码量对比。
- **扩展 ranges 侧**：给 `MyContainer` 加 `rbegin()` / `rend()` / `data()`，用 `static_assert` 验证 `bidirectional_range` 和 `contiguous_range`。
- **阅读 range-v3 源码**：`include/range/v3/range/access.hpp` 有集中化的 `_cpo_t` 写法，思路已接近 `tag_invoke`。与本练习的独立实现对比，感受集中化带来的代码量减少。

## 验收点

- 能画出（或写出注释形式的）ranges CPO 三阶查找和 tag_invoke 单一入口的流程图。
- 能填写四维度对照表（如何新增定制点、ADL 屏蔽方式、错误消息定位、泛型扩展性）的两列。
- 代码验证两条定制路径都正确工作。
- 能清楚说出"`tag_invoke` 不是 C++ 标准，只是 stdexec/P2300 里的提案定制模式"。
- 能解释 ranges CPO 为什么没有采用 `tag_invoke` 集中入口（时间线：ranges 在 C++20，P1895 在 C++20 feature freeze 之后）。

## 观察点

- ranges CPO 和 `tag_invoke` 解决同一个根本问题（ADL 污染 + 可扩展性），区别是抽象层次——CPO 每个独立管理，`tag_invoke` 统一收敛。这不是"谁更好"，而是"谁更适合当前场景"。
- 给一个类型同时注册两套机制不仅可行，现实中也有必要——容器既需要支持 ranges 管道访问，又可能作为 stdexec sender 传递。两套机制的接入方式不重叠，互不干扰。
- `tag_invoke` 的扩展优势：新增定制点只需新建 tag struct；用户接入方式统一，框架侧代码量大幅减少。这是 stdexec 有几十个定制点还能保持接口一致性的原因。

## 常见坑

- **混淆访问 CPO 和算法 niebloid**：`ranges::begin/end/size` 是访问 CPO，开放成员/ADL 定制；`ranges::sort/find` 是算法 niebloid，重点是标准算法对象的一等值调用，不开放用户通过 ADL 替换算法体。
- **以为 `tag_invoke` 是 C++ 标准**：不是。P1895R0 是提案，`tag_invoke` 来自 stdexec/libunifex。C++26 的 `std::execution` 可能采纳某种形式，但目前不在任何已发布标准中。
- **以为 ranges CPO 不能做集中入口**：ranges 标准库目前每个 CPO 各自独立，但理论上可以用类似 `tag_invoke` 的集中入口重新实现——range-v3 已做了部分集中化。"不集中"是实现选择，不是机制限制。

## 复盘问题

- `tag_invoke` 用一个 ADL 名字 + N 个 tag，比 ranges CPO 用 N 个独立 ADL 查找，实际大项目里冲突风险差异有多大？能举出 `tag_invoke` 还是会有冲突的场景吗？
- 在 `MyContainer` 同时注册两套定制的练习里，两套机制的"接入代码量"差异是什么？哪套更容易被第三方库的类型接入（你不拥有源码的情况）？
- ranges CPO 的"成员优先"策略和 stdexec 的 member-first dispatch（P2855 方向）有何相似？两者的出现顺序能说明什么演进规律？

## 对应官方参考

- P1895R0: tag_invoke: A general pattern for supporting customisable functions
- P2300R10: `std::execution` 提案中 CPO 的定义和 `tag_invoke` 的使用
- P2855R1: Member customization points for Senders and Receivers
- cppreference: [std::ranges::begin](https://en.cppreference.com/w/cpp/ranges/begin)

## Author-validation note

本题是观察型练习，CMake 使用 `ranges_add_observation(E3_cpo_tagdispatch_compare main.cpp)`。当前 `main.cpp` 给出 ranges 成员/ADL 风格与最小 `tag_invoke` 风格的完整可运行对照，保持 stdlib-only，不依赖 stdexec 实现库。

版本桥接要说清：ranges CPO 是 C++20 标准库机制；`tag_invoke` 是 P1895/stdexec/libunifex 传播出的协议风格，不是 C++20/23 标准；C10 的 execution 内容会继续讨论 P2300 及后续 member customization 方向。本题只建立 C04/C10 之间的概念桥，不把历史 `tag_invoke` 当成当前 ranges 定制协议。
