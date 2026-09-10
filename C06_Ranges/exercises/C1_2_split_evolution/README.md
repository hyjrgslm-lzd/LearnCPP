> 对应章节：04-模块C1-结构适配器 · 练习 C1-2：split 的 C++20 vs C++23 差异

# C1-2：split 的 C++20 vs C++23 差异

## 目标

直面标准演化带来的同名不同义：`views::split` 在 C++20 和 C++23 中的语义截然不同。
亲手验证两者的子范围类型差异，理解 P2210R2 "改名保留旧语义"的实际效果。

## 前置理解

- 了解 C++20 的 `views::split` 产出的子范围不是 `forward_range` 以上，
  子范围不能直接构造 `std::string`。
- 了解 P2210R2 的处理方式：把 C++20 的 `views::split` 改名为 `views::lazy_split`，
  C++23 新的 `views::split` 是重新设计的版本，子范围保留底层 contiguous 性质。
- 接受本题的目的不是"选哪个更好"，而是"亲眼看到差异"。

## 预计练习方向

1. 在 C++23 编译器下用 `views::lazy_split(',')` 切分 `"hello,world,ranges"`，
   手动逐字符打印各子段，确认输出 `hello|world|ranges|`。
   用 `static_assert` 验证子范围是 `forward_range` 而非 `contiguous_range`。
2. 用 C++23 新 `views::split(',')` 切分同一字符串，
   用 `string_view(subr.begin(), subr.end())` 直接构造子串并打印。
   用 `static_assert` 验证子范围是 `contiguous_range`。
3. 对比两种子范围类型差异，理解代理迭代器（lazy_split）
   与透传底层迭代器（new split）的本质区别。
4. 在注释里解释 P2210R2 为何选择"改名保留"而非"删除旧版本"。

## 进阶预计方向

- 用 `lazy_split` 切分 `forward_list<int>`（分隔值 0），验证子范围迭代器
  随底层类型降为 `forward_iterator`。
- 解释为什么 `std::string(subr.begin(), subr.end())` 对 `lazy_split` 子范围
  能编译，而 `std::string_view(subr.begin(), subr.end())` 不能。
  （InputIterator 构造 vs contiguous_iterator 要求）
- 探索 C++23 `split` 对 `forward_range` 底层时的迭代器概念传递。

## 验收点

- 能分别写出 `lazy_split` 和 C++23 `split` 的代码，并观察子范围类型差异。
- 能解释为什么 C++20/lazy_split 子范围不能直接构造 `std::string`，
  而修订后 split 可以。
- 能指出 P2210R2 "改名保留旧语义"的实际效果：两者在 C++23 下都可用，只是名字不同。
- 能说出修订后 split 对输入 range 的概念要求，以及子范围迭代器类别由底层继承的机制。

## 观察点

- `views::lazy_split` 子范围是惰性代理：迭代器本质是"以分隔符为边界按需推进的游标"，
  只能满足 `forward_iterator`，无法向上传递底层 contiguous 性质。
- C++23 新 `views::split` 子范围是 `subrange<iterator_t<V>, iterator_t<V>>`，
  迭代器类型直接取自底层。底层是 `string_view`（contiguous）时，
  子范围 `begin()` 就是 `const char*`，`string_view` 构造自然成立。
- P2210R2 的改动不是破坏性重做：旧语义通过 `lazy_split` 继续可用，
  对维护 C++20 代码库而言，升级到 C++23 后行为不变，只需更新名字为 `lazy_split`。
- 跨编译器行为不一致：MSVC 和 GCC 对 P2210R2 DR 与 lazy_split 实现进度可能不同。
  编译失败不等于"标准说错了"。

## 常见坑

- 在 C++23 环境下把 `views::split` 当 C++20 的 `split` 用，
  误以为子范围是 forward-only 的——实际上子范围已经是 contiguous 的了。
- 在 C++20 环境下误判能否用迭代器对构造字符串：
  `std::string(it, it)` 只要求 InputIterator（能编过），
  `std::string_view(it, it)` 要求 contiguous_iterator（不能）。
- 混淆 `lazy_split` 和 `split` 的适用场景：
  需要 `string_view` 化子范围时用 C++23 `split` 更自然；
  只是简单迭代子范围内元素时两者都可以。
- 期待 `split` 分隔符可以是复杂谓词：`split` 的分隔符是值或 range 模式，
  不是任意谓词；谓词切分应用 `views::chunk_by`（模块 D 覆盖）。

## 提示

- 如果编译器是 C++20 only，可以用 `views::split` 做 lazy_split 验证，
  先读懂修订后 split 语义，等升级编译器后再验证。
- 当子范围需要转换为 `string` 或 `string_view` 时，先打印子范围迭代器类型，
  直观看到 lazy_split vs split 的差异。
- `static_assert(!std::ranges::contiguous_range<decltype(lazy_first)>)` 是
  不运行就能看出概念差异的直接验证手段。

## 复盘问题

1. P2210R2 为什么选择“旧语义由 lazy_split 承接、split 名字给修订语义”而不是保留旧名字？
   这个决定对现有代码的迁移有什么影响？
2. `lazy_split` 子范围迭代器是"代理迭代器"，C++23 `split` 子范围迭代器是
   底层 range 的真实迭代器。两种设计在"传递 contiguous 性质"上的核心差异是什么？
3. 为什么 `split` 分隔符用"值"或"range 模式"而不是"谓词"？
   想要谓词切分应该用什么适配器？
4. 维护一个 C++20 代码库，升级编译器到 C++23 后需要做什么改动？

## 对应官方参考

- P0896R4：C++20 核心合入，包含原始 `split_view`
- P2210R2：C++23 `views::split` 语义改进，子范围可构造 string
- cppreference: [std::ranges::split_view](https://en.cppreference.com/w/cpp/ranges/split_view)
- cppreference: [std::ranges::lazy_split_view](https://en.cppreference.com/w/cpp/ranges/lazy_split_view)
## 参考解析

预测：P2210R2 已作为 C++20 DR 采纳，不能靠 `-std=c++20` 假定旧 split 行为。旧惰性设计由 `lazy_split` 承接；修订后 `split` 的子范围用底层真实迭代器，连续底层上可直接构造 `string_view`。

当前程序同时观察 `lazy_split` 的 forward-only 代理子范围和修订后 `split` 的 contiguous 子范围，并把两者都物化成 token。扩展时要分清：`std::string(first,last)` 只需 input iterator；真正体现差异的是 `string_view` 构造需要 contiguous/sized。
