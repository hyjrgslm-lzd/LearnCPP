> 对应章节：模块 H — 高级实现模式 / H-2 `common_iterator` + proxy iterator 的 `iter_move` / `iter_swap`

## 目标

实现两个标准库内部常见桥接模式：

- `my_common_iterator<I, S>`：把 iterator/sentinel 异型范围包装成 begin/end 同类型，供 C++17 风格算法使用。
- `ZipIterator<It1, It2>`：演示 proxy reference 下必须显式定制 `iter_move` / `iter_swap`。

本题验证四条路径：

- `src/student/common_proxy.hpp`：学生可编辑入口。
- `src/reference/common_proxy.hpp`：标准答案。
- `validation/good/common_proxy.hpp`：独立正确实现。
- `validation/bad/common_proxy.hpp`：安全但真实错误的实现，`iter_swap` 不交换底层元素，必须被 checker 拒绝。

## 必做任务

1. 实现 `my_common_iterator<I, S>`。
   - 内部用 `std::variant<I, S>` 保存 begin 端或 end 端。
   - `operator*` 只在持有 `I` 时解引用。
   - `operator++` 推进 `I`。
   - `operator==` 支持 `I/S`、`S/I`、`S/S` 三类比较，让 `std::accumulate(first, last, init)` 能消费它。

2. 实现 `ZipIterator` 的 proxy 解引用。
   - `operator*` 返回 `std::tuple<T1&, T2&>`。
   - 这不是元素对象本体，而是一个按值返回的 proxy。

3. 定制 `iter_move`。
   - 返回 `std::tuple<std::iter_rvalue_reference_t<It1>, std::iter_rvalue_reference_t<It2>>`。
   - 每一维都通过 `std::ranges::iter_move` 转发到底层迭代器。

4. 定制 `iter_swap`。
   - 对两条底层 iterator 分别调用 `std::ranges::iter_swap`。
   - 不依赖“交换 proxy 外壳”的偶然行为。

## 验收点

- `my_common_iterator` 能被 `std::accumulate` 消费，1 到 10 求和结果为 55。
- `ZipIterator` 解引用类型是 `std::tuple<int&, double&>`。
- `std::ranges::iter_move(it)` 的类型是 `std::tuple<int&&, double&&>`。
- `std::ranges::iter_swap(a, b)` 同时交换两条底层序列。
- `validation/bad` 的 no-op `iter_swap` 被行为 checker 拒绝。

## 常见坑

- 把 `iter_move` / `iter_swap` 写成普通成员函数。ranges CPO 不走成员查找，应该写 hidden friend 或同命名空间非成员。
- 只交换 zip 的一维，导致 keys 和 values 不再保持同行关系。
- 给 proxy iterator 声明过强的 `iterator_category`，让老算法按真实引用语义使用它。
- 把 `views::common` 与 `views::as_const` 混淆：前者统一 begin/end 类型，后者控制元素可变性。

## 对应参考

- `references/implementation-spec.md`
- P0896R4：`common_iterator`
- P2321R2：`zip_view` 的 `iter_move` / `iter_swap`
- cppreference：`std::common_iterator`、`std::ranges::iter_move`、`std::ranges::iter_swap`
