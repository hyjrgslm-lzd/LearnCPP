# 练习 B-1：递归 generator 与栈

> 详尽版本见 `../../03-模块B-generator与task的使用.md` 的 `练习 B-1` 章节。
> 本 README 仅摘抄"目标 / 必做任务 / 验收点"。

## 目标

用递归 generator 实现二叉树的中序遍历（惰性 yield 每个节点值），亲手观察对称传输
（symmetric transfer）如何在编译器层面防止递归 yield 爆栈。

> 关键纠正：**仅 P2502R2 的 `co_yield std::ranges::elements_of(inner_gen)` 才触发
> symmetric transfer。** 普通的 `for (int v : inner_gen) co_yield v;` 不触发——它是
> 普通迭代器层层 resume，深度退化树仍会爆栈。

## 必做任务

1. 定义 `Node{value, left, right}`，构造一棵深度至少 5 层的满二叉树（≥20 节点）。
2. 写两个版本的 `inorder(Node*)`：
   - **版本 A（推荐）**：`co_yield std::ranges::elements_of(inorder(left));` —— 触发 symmetric transfer。
   - **版本 B（反例）**：`for (int v : inorder(left)) co_yield v;` —— 普通 resume 链。
3. 用 range-based for 验证两个版本输出相同的中序序列。
4. 在递归入口和每个 `co_yield` 前后加日志，观察最左叶子被 yield 时的执行顺序。
5. 画一张调用图：最左叶子 yield 时父帧分别挂在哪。

## 进阶任务

- 构造退化树（深度 200/500）：分别用版本 A / B 遍历——A 不爆栈，B 与普通递归一样爆栈。
- 自定义 generator + `yield_from` 风格 promise（`await_transform`）。
- 在 GCC 用 `-fdump-tree-coro`，Clang 用 `-Xclang -ast-dump`，观察协程帧布局。

## 验收点

- 中序输出正确。
- 日志能展示深层左子树先 yield，浅层节点处于"等待下一个值"的冻结状态。
- 你能说明版本 B 在退化树上为什么爆栈、版本 A 为什么不爆栈。
- 你能画出退化树遍历时协程帧之间的 resume 链。
