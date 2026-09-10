# 05 关联容器：等价关系、索引与查找成本

日志主案例已经有了 `vector<LogRecord>`。如果问题是“按时间顺序扫描”，`vector` 很合适；如果问题变成“某个用户出现过几次”“是否出现过某个 level”“按 user_id 有序导出”，每次全表扫描就把查询成本交给了调用方。关联容器的作用是把“怎么定位元素”变成容器自己的不变量。

本章只讨论标准容器的使用契约和可观察成本，不把 `std::map` 写成某一种树。当前 MSVC STL 的 `std::map` 实现可从 `xtree` 读到红黑树旋转和颜色修复；本课后面的 AVL 只是教学实现，用高度和平衡因子解释旋转，不等同于 `std::map` 的实现。

## `map` 与 `set` 保存的是关系

`std::map<Key, T, Compare>` 的键唯一。这里的“唯一”不是 `operator==`，而是比较器定义的等价关系：

```cpp
!comp(a, b) && !comp(b, a)
```

如果 `Compare` 是大小写不敏感比较器，`"Alice"` 和 `"alice"` 可能就是同一个键；如果 `operator==` 仍区分大小写，两个关系会冲突。查找、插入、删除都依赖同一个比较器关系，不能用另一个相等规则解释结果。

`std::set<Key>` 可以看成只有键没有值的 `map`。它适合表达“集合成员资格”：某个用户是否出现、某个告警级别是否启用。`std::map` 适合表达“键到值”：用户到计数、时间戳到记录。`std::multimap` 和 `std::multiset` 允许等价键重复，适合“一键多值”：一个用户对应多条日志。重复键的区间用 `equal_range` 取出；不要先 `find` 一个元素再手写向两边猜边界。

## 修改键必须离开再回来

树节点的位置由插入时的键决定。直接修改容器内部的键会破坏有序不变量，所以 `map` 的键是 `const`。C++17 的 node handle 给了一个受控出口：

```cpp
auto node = counts.extract("alice");
node.key() = "alice.renamed";
counts.insert(std::move(node));
```

节点被 `extract` 后暂时不属于容器，才能修改键；插回时重新按比较关系定位。这个过程还有 allocator 前提：跨容器插入 node handle 要求 allocator 兼容。忽略这个前提不是“优化”，而是离开标准契约。教学代码里只在同一个容器内改键，避免引入 allocator 子课题。

## 透明查找减少临时对象

普通 `std::map<std::string, int>` 用 `"alice"` 查找时可能构造临时 `std::string`。如果比较器是 `std::less<>`，并且参与比较的类型能互相比较，就可以异构查找：

```cpp
std::map<std::string, int, std::less<>> counts;
auto it = counts.find(std::string_view{"alice"});
```

透明查找的核心不是“更快”三个字，而是调用表达式不用先造 `Key`。前提是比较器声明透明，并且所有参与比较的表达式满足严格弱序。`unordered_map` 的透明查找还要求 hash 和 equal 同时透明，并且相等对象必须给出相同 hash；只透明一个不够。

## `unordered_*` 是 hash/equal 的协议

`std::unordered_map<Key, T, Hash, KeyEqual>` 先用 hash 找桶，再用 equal 判断同桶候选。必要条件是：

```text
eq(a, b) 为 true  =>  hash(a) == hash(b)
```

反过来不要求成立。所有键 hash 到同一个桶仍然可以正确，只是平均常数时间退化成线性链扫描。本课 L07 会手写一个有限链式哈希表，专门让多个键落到同一桶，观察正确性和 `rehash` 的区别。

`rehash` 改的是桶结构，不是把值“复制成另一个含义”。标准容器里引用和指针稳定性、迭代器失效、实现是否移动节点，是三个不同层次的问题。课程正文只把标准保证写成保证；MSVC 源码观察只写成当前实现观察。

## 主案例中的选择

日志主案例里有三类索引：

- 全部记录仍放在 `vector<LogRecord>`，因为排序、切片和 ranges 管道需要连续序列。
- 用户计数用 `std::map<std::string, int>`，因为结果需要稳定有序输出。
- 如果只做存在性或大量点查，`std::unordered_set<std::string>` 更直接；但要同时记录碰撞和 rehash 对迭代器的影响。

选择容器时先写问题：要唯一键还是重复键，要有序遍历还是点查，要稳定引用还是紧凑连续，要按比较等价还是 hash/equal。没有这个问题，容器名字只是猜。

## L03 观察题

`L03_associative_containers` 覆盖 `map/set/multimap/unordered_map` 的小场景：比较等价、重复键区间、透明查找、node handle 改键、碰撞和 rehash。它不要求实现容器；真正的哈希实现放到 L07。

解析重点：

1. `map` 的重复插入是否成功由比较器等价决定。
2. `multimap::equal_range` 返回的是等价键区间。
3. 透明查找要求比较器或 hash/equal 明确透明。
4. node handle 改键要先 `extract`，再 `insert`。
5. rehash 改桶数和桶分布，不改变键值对语义。
