# L08 AVL tree

对应章节：`chapters/07-heap-hash-avl.md`。只编辑 `src/student/avl_set.hpp`。

实现 `c06_l08::AvlSet`，有限 API：

- `bool insert(int value)`
- `bool contains(int value) const`
- `std::vector<int> inorder() const`
- `int height() const`
- `bool is_balanced() const`
- `std::size_t size() const`

要求：重复插入返回 `false`；中序遍历升序无重复；LL/RR/LR/RL 四种插入形态都保持高度字段正确和平衡因子不超过 1。

只读结构 inspector 也是本题契约的一部分：

- `const Node* inspect_root() const`
- `static int key(const Node*)`
- `static const Node* left(const Node*)`
- `static const Node* right(const Node*)`
- `static int stored_height(const Node*)`

checker 不信 `is_balanced()` 自报，会沿 inspector 递归重算 BST 上下界、循环、节点数、实际高度、stored height 和每个节点平衡因子。

解析：递归 `insert_node` 必须返回修复后的子树根，父节点或 `root_` 立刻接住这个返回值。`height(nullptr)=0`，`refresh(node)=max(left,right)+1`。右旋顺序是 `B=A.left; A.left=B.right; refresh(A); B.right=A; refresh(B); return B`，左旋对称。LR 是先左旋左孩子再右旋当前节点，RL 是先右旋右孩子再左旋当前节点。bad 控制体只处理单旋，不处理 LR/RL，会被 `lr rotation keeps sorted balanced tree` 拒绝；额外 fake vector 和 stale height 坏例分别验证 checker 真的看结构。
