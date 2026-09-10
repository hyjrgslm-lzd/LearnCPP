# L04 algorithms

对应章节：`chapters/06-algorithm-contracts.md`。

本题是观察型程序，覆盖遍历、查找、修改、partition、稳定排序、二分、merge、集合、heap、erase-remove、fold 和 projection。它不要求实现算法。

解析：

- 二分、merge 和集合算法要求输入已按同一比较口径排序。
- `partition` 不保留分区内顺序，`stable_partition` 保留。
- heap 的 `front()` 是最大值，但整个数组不是全序。
- `remove_if` 只返回逻辑尾，`erase` 才改变容器大小。
- projection 在算法内部先于谓词/比较器执行。
