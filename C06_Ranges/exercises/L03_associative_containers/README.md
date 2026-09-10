# L03 associative containers

对应章节：`chapters/05-associative-containers.md`。

本题是观察型程序，不提供 Student。运行目标 `L03_associative_containers`，观察 `map/set/multimap/unordered_map` 的比较等价、透明查找、node handle、碰撞和 rehash。

解析：

- `std::map` 的唯一性由 `Compare` 的等价关系决定。
- `multimap::equal_range` 是重复键的主入口。
- 透明查找要透明比较器，unordered 还要透明 hash 与 equal 同时成立。
- node handle 改键先离开容器，再按不变量插回。
- 碰撞合法；rehash 后语义不变，桶分布变化。
