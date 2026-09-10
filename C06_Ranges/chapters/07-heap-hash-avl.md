# 07 三个小实现：堆、链式哈希与 AVL

本章实现三个有限数据结构：二叉最大堆、链式哈希表、AVL 集合。它们是教学模型，不是 STL 替代品。API 少、类型固定、容量或内存模型明确，才能把不变量讲清楚。

## 二叉堆：数组里的局部不变量

最大堆用 `vector<int>` 存一棵完全二叉树。下标 `i` 的父节点是 `(i - 1) / 2`，左右孩子是 `2*i + 1` 和 `2*i + 2`。不变量是每个父节点都不小于自己的孩子：

```text
parent >= left child
parent >= right child
```

这不等于数组有序。`[9, 7, 8, 1, 3, 2]` 是堆，但不是降序数组。`push` 把新元素放到末尾，然后向上交换，直到父节点不再更小。循环边界很小：根节点没有父节点；每次交换后下标变成父节点，所以最多走树高次。

`pop_max` 的关键是下沉时必须选择两个孩子里较大的那个：

```text
result = data[0]
data[0] = data.back()
remove last
while left child exists:
    child = larger(left, right if exists)
    if data[child] <= data[index]: stop
    swap(data[index], data[child])
    index = child
```

如果永远选左孩子，右孩子更大时根部不变量会残留错误；如果相等时继续交换，虽然仍可能正确，但会做无意义移动。空堆不进入这段逻辑；单元素堆删除后没有根需要下沉；容量满时 `push` 应该拒绝而不是扩容，因为本题训练的是固定容量堆。

L06 的 `IntMaxHeap` 只支持固定容量、`push(int) -> bool`、`peek_max()`、`pop_max()`、`size()`、`empty()`。容量满时 `push` 返回 `false`，不抛异常；空堆 `peek/pop` 返回 `std::optional<int>{}`。checker 覆盖空、满、重复值和降序弹出。

## 链式哈希：正确性先于平均复杂度

链式哈希表用桶数组保存链表或小 vector。插入时先算 hash，再取桶下标；同桶里用 equal 找等价键。碰撞不是错误：两个不同键可以落到同一个桶。真正的错误是丢掉同桶元素、重复插入等价键却不更新、或者 rehash 后查不到旧键。

L07 的 `StringIntMap` 只支持 `std::string` 键和 `int` 值，公开：

```cpp
explicit StringIntMap(std::size_t bucket_count);
bool put(std::string key, int value);
std::optional<int> get(std::string_view key) const;
bool erase(std::string_view key);
void rehash(std::size_t bucket_count);
std::size_t size() const;
std::size_t bucket_count() const;
std::size_t bucket_size(std::size_t bucket) const;
```

`put` 返回是否插入了新键；等价键存在时更新值但不增加 size。`rehash` 只重建桶分布，保留所有键值对。教学实现采用先收集旧桶、准备新桶、全部重新插入成功后再提交的顺序：

```text
old = move(buckets)
new_buckets = bucket_count empty buckets
for each old entry:
    place entry into new_buckets[bucket_for(entry.key, new_count)]
commit buckets = new_buckets
```

这个顺序表达强保证的形状：失败时旧表仍有恢复来源；成功后 size 与每个 key/value 不变。标准 `unordered_map` 的具体节点搬迁和引用稳定性另有标准契约，本题只要求自己的有限表做到 rehash 后可查、不丢碰撞链、不重复计数。

## AVL：用高度选择旋转

AVL 树是二叉搜索树加高度不变量。约定 `height(nullptr) = 0`，叶子高度为 1。每个节点保存 `stored_height`，任一节点左右子树高度差的绝对值不超过 1。插入先按二叉搜索树规则走到叶子，再沿递归回溯路径更新高度并修复失衡点。

递归插入的返回值必须是“修复后的子树根”。这点比旋转名字更重要：

```text
insert_node(node, value):
    if node is null: return new Node(value), inserted=true
    if value < node.key: node.left = insert_node(node.left, value)
    else if node.key < value: node.right = insert_node(node.right, value)
    else: inserted=false; return node

    refresh(node)
    return rebalance(node)
```

调用方总是把返回值写回自己的 child 或 root。这样无论发生单旋还是双旋，父节点都指向新的子树根。重复插入不创建节点，也不应该沿途改变 size；高度可以重新 refresh，但结构和值集合不变。

`refresh(node)` 必须在孩子已经接回之后做：

```text
node.height = max(height(node.left), height(node.right)) + 1
balance = height(node.left) - height(node.right)
```

高度先错，后面的旋转选择也会错。checker 因此不信 `is_balanced()` 的返回值，而是通过只读 inspector 自己递归重算实际高度、BST 上下界、节点数和每个节点的平衡因子。

四种旋转来自插入路径：

- LL：插入到左孩子的左子树，当前节点 `A` 右旋，`B=A.left` 成为新根。
- RR：插入到右孩子的右子树，当前节点 `A` 左旋，`B=A.right` 成为新根。
- LR：插入到左孩子的右子树，先让 `A.left` 左旋，再让 `A` 右旋。
- RL：插入到右孩子的左子树，先让 `A.right` 右旋，再让 `A` 左旋。

右旋的重连顺序如下，左旋完全对称：

```text
rotate_right(A):
    B = A.left
    A.left = B.right
    refresh(A)
    B.right = A
    refresh(B)
    return B
```

先 refresh 旧根 `A`，再 refresh 新根 `B`。因为 `B.height` 依赖已经接到右侧的 `A.height`。LR/RL 不是“换一个方向的单旋”；它们先修复孩子的内侧重，再修复当前节点。只写 LL/RR 单旋，`30, 10, 20` 或 `10, 30, 20` 这类序列会保持中序正确，但结构仍不平衡。

旋转只改变指针结构，不能改变中序序列。检查 AVL 不能只看 `contains`，还要检查中序有序、无重复、高度字段正确和平衡因子。L08 的 `AvlSet` 只支持 `insert(int) -> bool`、`contains(int)`、`inorder()`、`height()`、`is_balanced()`。重复插入返回 `false`。

## 与标准容器的边界

`std::priority_queue` 通常用堆组织，但标准暴露的是适配器契约，不暴露底层堆数组。`std::unordered_map` 的桶、节点和 rehash 细节由实现选择；L07 只讲链式方案。`std::map` 在当前 MSVC STL 中走红黑树，本课 L08 走 AVL。它们都维护有序查找，但修复规则不同，不能互相替换解释。

这些实现题的正确交付标准是：Reference 和 good 独立通过，bad 被 checker 用稳定诊断拒绝，Student 初态安全失败。能编译不等于完成实现；能查到一个元素也不等于不变量成立。
