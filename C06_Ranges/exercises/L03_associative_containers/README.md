# L03 associative containers

对应章节：`chapters/05-associative-containers.md`。

本题是观察型程序，不提供 Student。运行目标 `L03_associative_containers`，观察 `map/set/multimap/unordered_map` 的比较等价、透明查找、node handle、碰撞和 rehash。

解析：

- `std::map` 的唯一性由 `Compare` 的等价关系决定。
- `multimap::equal_range` 是重复键的主入口。
- 透明查找要透明比较器，unordered 还要透明 hash 与 equal 同时成立。
- node handle 改键先离开容器，再按不变量插回。
- 碰撞合法；rehash 后语义不变，桶分布变化。
## IDE 项目

生成 Visual Studio 工程后，启动项目是 `L03_associative_containers`。

本题是观察题，没有本题内 `src/student/`、`src/reference/` 或 `validation/` 变体。
- 源码入口：`main.cpp`。

单题独立构建：从本目录运行 cmake -S . -B build/leaf -G "Visual Studio 18 2026" -A x64，然后 cmake --build build/leaf --config Debug --target L03_associative_containers。
修改后先重建 `L03_associative_containers`，再按课程 `BUILD_GUIDE.md` 运行对应检查。
