# L04 algorithms

对应章节：`chapters/06-algorithm-contracts.md`。

本题是观察型程序，覆盖遍历、查找、修改、partition、稳定排序、二分、merge、集合、heap、erase-remove、fold 和 projection。它不要求实现算法。

解析：

- 二分、merge 和集合算法要求输入已按同一比较口径排序。
- `partition` 不保留分区内顺序，`stable_partition` 保留。
- heap 的 `front()` 是最大值，但整个数组不是全序。
- `remove_if` 只返回逻辑尾，`erase` 才改变容器大小。
- projection 在算法内部先于谓词/比较器执行。
## IDE 项目

生成 Visual Studio 工程后，启动项目是 `L04_algorithms`。

本题是观察题，没有本题内 `src/student/`、`src/reference/` 或 `validation/` 变体。
- 源码入口：`main.cpp`。

单题独立构建：从本目录运行 cmake -S . -B build/leaf -G "Visual Studio 18 2026" -A x64，然后 cmake --build build/leaf --config Debug --target L04_algorithms。
修改后先重建 `L04_algorithms`，再按课程 `BUILD_GUIDE.md` 运行对应检查。
