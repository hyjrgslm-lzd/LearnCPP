# 练习 B-1：递归 generator 与栈

先读 [模块 B 的递归 generator 章节](../../03-模块B-generator与task的使用.md#b1)。本题关注生产者嵌套，不只是中序遍历算法。

## Part 1：同步中序语义

打开 [main.cpp](main.cpp)，构造一棵二叉树。中序遍历的顺序是左子树、当前节点、右子树。先用小树确认预期输出，再进入 generator 写法。

## Part 2：递归 generator

补全 `inorder_v2_for_yield(Node*)`：左子树用 `for (int v : inorder_v2_for_yield(root->left)) co_yield v;` 转发，当前节点 `co_yield root->value`，右子树同理。

观察最左叶子产出时，父 generator 停在自己的左子树循环里。每一层递归调用都是一份独立协程状态。

## Part 3：`elements_of` 路径

如果当前标准库支持 `std::ranges::elements_of`，补全 `inorder_v1_elements_of`：

```cpp
co_yield std::ranges::elements_of(inorder_v1_elements_of(root->left));
co_yield root->value;
co_yield std::ranges::elements_of(inorder_v1_elements_of(root->right));
```

它表达“把子 generator 的元素接到当前 generator 输出中”。若当前工具链不支持，记录编译器/标准库限制，用 [solution.cpp](solution.cpp) 的普通递归 generator 与显式栈版本验证中序语义。

## Part 4：画嵌套图

画出最左叶子被 yield 时的状态：

```text
root generator -> 等 left generator
left generator -> 等 deeper left generator
leaf generator -> co_yield leaf.value
```

标出每个父帧暂停的位置。退化树实验只要求画控制流并分析栈增长风险；验证标准是恢复链和父子 generator 的转发关系。

**答案解析：** 示例图中至少要有三类关系：每个 generator 调用各自创建一份协程帧，父帧停在“转发左子树”的循环或 `elements_of` 位置，叶子帧停在 `co_yield leaf.value`。最外层消费者推进一次，会沿 root -> left -> leaf 的恢复链找到当前值；消费完这个值后，再沿相反方向返回到父层继续中序流程。若树退化成链，递归 generator 仍会形成一串父子 generator 等待关系，显式栈 reference 则把这串关系放进一个容器里迭代处理。

## 验收

- 中序输出正确。

  **答案解析：** 中序遍历的顺序固定为左子树、当前节点、右子树。对小树先写出手工序列，再比较 generator 输出；如果两者一致，说明转发左子树、yield 当前节点、转发右子树的顺序没有写反。

- 你能说明每层递归 generator 都是独立协程实例。

  **答案解析：** 每次调用 `inorder(root->left)` 或 `inorder(root->right)` 都会创建新的 generator 返回对象和协程帧。父层只持有并消费子 generator，子层自己的 `root` 参数、当前位置和当前 yield 值都在自己的帧里。它们共享的是控制流关系，不是同一份协程状态。

- 你能区分 `elements_of` 递归产出和 `for + co_yield` 用户层转发。

  **答案解析：** `elements_of(child)` 让 generator 机制直接把 child 的元素拼接到当前输出中，表达的是“转发整个子 range”。`for (int v : child) co_yield v;` 是用户代码逐个取 child 的值再 yield，一样能得到中序结果，但多了一层显式循环。工具链不支持 `elements_of` 时，用 `for + co_yield` 验证语义即可。

- 你能解释显式栈 reference 为什么能避免递归调用栈增长。

  **答案解析：** 显式栈版本把待访问节点放进 `std::vector` 或 `std::stack`，用循环推进状态，不再通过函数递归或递归 generator 形成一层层调用/等待链。树越深，增长的是容器里的节点记录，而不是 C++ 调用栈深度。它仍有 O(height) 存储成本，但这个成本由显式数据结构承载。

## Student 检查

`main.cpp` 会把两种 generator 输出收集成 `std::vector<int>`，再和同步中序遍历结果比较：

| Part | 操作 | 本地检查 |
| --- | --- | --- |
| Part 1 | 构造小树并确认中序顺序 | `collect_inorder(root, expected)` 生成基准序列 |
| Part 2 | `for + co_yield` 递归 generator | `v2 == expected` |
| Part 3 | `elements_of` / symmetric-transfer 路径 | `v1 == expected` |
| Part 4 | 嵌套图 | 仍作为文字解析；不把图形作业伪装成自动测试 |

完成前：只 yield 当前节点的占位实现会因为序列不完整而失败
