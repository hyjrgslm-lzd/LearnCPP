> 对应章节：模块 H — 高级实现模式 / H-1 `__non_propagating_cache` + `my_filter_view` begin 缓存

## 目标

把模块 G 的 `my_filter_view` 升级到实现级语义：`begin()` 第一次扫描并缓存首个满足谓词的位置，后续 `begin()` 复用缓存；拷贝或移动 view 时缓存不传播，新的 view 必须重新扫描。

本题验证四条路径：

- `src/student/filter_cache.hpp`：学生可编辑入口。
- `src/reference/filter_cache.hpp`：标准答案。
- `validation/good/filter_cache.hpp`：独立正确实现，用来证明 checker 没有只认 Reference。
- `validation/bad/filter_cache.hpp`：安全但真实错误的实现，缓存被拷贝传播，必须被 checker 拒绝。

## 必做任务

1. 实现 `non_propagating_cache<T>`。
   - 默认构造为空。
   - 拷贝构造、移动构造都产生空缓存。
   - 拷贝赋值、移动赋值都清空目标缓存。
   - `emplace`、`reset`、`has_value`、`operator*` 提供最小 optional 包装接口。

2. 在 `my_filter_view::begin()` 中使用缓存。
   - 第一次调用从 `ranges::begin(base_)` 扫描到第一个满足谓词的位置。
   - 第二次调用直接返回缓存。
   - `begin()` 保持非 `const`，因为 forward-range 缓存会修改 view 自身状态。

3. 实现 filter iterator 推进。
   - `operator++` 先推进一位，再跳过不满足谓词的元素。
   - 对 bidirectional 底层，`operator--` 向前找到上一个满足谓词的元素。

## C++26 const filter 口径

不要再说“任意 `filter_view` 都不能 const 迭代”。N5047 LWG Poll 13 以 DR 采纳 P3725R3 后，标准新增一条受限分支：当 `const V` 是 input range、`const V` 不是 forward range、且 predicate 可在 const 下调用时，`filter_view` 可以提供 `const begin/end`。

本题覆盖的是普通 forward 底层（例如 `std::vector<int>`）的缓存分支。该分支仍需要缓存首个满足谓词的位置，因此 `const filter_view` 不能调用普通缓存 `begin()`。checker 中的 `static_assert` 专门验证这个边界。

## 验收点

- 第一次 `begin()` 会扫描到底层第一个满足谓词的元素。
- 第二次 `begin()` 不重复扫描。
- 拷贝后的 view 不继承缓存，第一次 `begin()` 会重新扫描。
- `validation/bad` 的“缓存传播”实现被行为 checker 拒绝。
- 能解释 forward 缓存分支与 C++26 input-only const 分支的区别。

## 常见坑

- 用普通 `std::optional<It>` 直接拷贝缓存，导致新 view 复用旧 view 的已扫描状态。
- 为了让 `begin()` 变成 `const` 而使用 `mutable optional`，却没有同步修正拷贝语义。
- 每次 `begin()` 都重新扫描，功能结果看似正确，但没有实现标准里的缓存行为。
- 把 C++26 新增 input-only const 分支误读成“vector/filter 这类 forward 缓存分支也能 const begin”。

## 对应参考

- `references/implementation-spec.md`
- `references/standards.md`
- N5047 LWG Poll 13 / P3725R3：受限 `filter_view const begin/end` 分支。
