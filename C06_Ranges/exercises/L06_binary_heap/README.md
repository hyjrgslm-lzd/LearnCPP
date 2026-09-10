# L06 binary heap

对应章节：`chapters/07-heap-hash-avl.md`。只编辑 `src/student/finite_max_heap.hpp`。

实现 `c06_l06::IntMaxHeap`，有限 API：

- `explicit IntMaxHeap(std::size_t capacity)`
- `bool push(int value)`
- `std::optional<int> peek_max() const`
- `std::optional<int> pop_max()`
- `std::size_t size() const`
- `bool empty() const`

要求：固定容量，满时 `push` 返回 `false`；空堆 `peek_max/pop_max` 返回空；重复值合法；连续 `pop_max` 必须降序。

解析：`push` 末尾插入后向上交换；`pop_max` 用最后一个元素补根后向下交换。堆只保证父节点不小于孩子，不保证数组全序。bad 控制体故意按最小堆弹出，会被 `pop returns descending order` 拒绝。
