# L07 chained hash

对应章节：`chapters/07-heap-hash-avl.md`。只编辑 `src/student/chained_hash.hpp`。

实现 `c06_l07::StringIntMap`，有限 API：

- `explicit StringIntMap(std::size_t bucket_count)`
- `bool put(std::string key, int value)`
- `std::optional<int> get(std::string_view key) const`
- `bool erase(std::string_view key)`
- `void rehash(std::size_t bucket_count)`
- `std::size_t size() const`
- `std::size_t bucket_count() const`
- `std::size_t bucket_size(std::size_t bucket) const`

要求：碰撞必须保留全部键；重复 `put` 更新值但不增加 size；`rehash` 后所有键仍可查。bucket 数至少为 1。

解析：hash 只选择桶，equal 决定是否同键。bad 控制体在 `rehash` 时只保留每桶第一个元素，会被 `rehash keeps colliding keys` 拒绝。
## IDE 项目

生成 Visual Studio 工程后，启动项目是 `L07_chained_hash_student`。

本题是实现题。学习者只改 Student 入口；Reference、validation 和 checks 只用于对照与验证。
- Student 入口：`src/student/chained_hash.hpp`。
- Checker 入口：`checks/hash_checks.cpp`。
- Reference 对照：`src/reference/chained_hash.hpp`。
- validation/good 对照：`validation/good/chained_hash.hpp`。
- validation/bad 反例：`validation/bad/chained_hash.hpp`。

单题独立构建：从本目录运行 cmake -S . -B build/leaf -G "Visual Studio 18 2026" -A x64，然后 cmake --build build/leaf --config Debug --target L07_chained_hash_student。
修改后先重建 `L07_chained_hash_student`，再按课程 `BUILD_GUIDE.md` 运行对应检查。
