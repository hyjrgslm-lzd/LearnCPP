# L05 dynamic_array

实现一个教学版 `c06_l05::dynamic_array<T>`。它只覆盖连续原始存储、尾部构造、增长、提交/回滚、移动和 `span` 借用观察，不承诺 `std::vector` 兼容。

公开接口：

- 默认构造、析构。
- 禁止复制。
- `noexcept` 移动构造和移动赋值。
- `size()`、`capacity()`、`empty()`。
- `operator[]` const/non-const。
- `view()` const/non-const，返回 `std::span`。
- `reserve(std::size_t)`、`push_back(T)`、`pop_back()`、`clear()`。

类型边界：`T` 非 cv，析构 `noexcept`，并且可复制或 `nothrow` 移动构造。空 `pop_back()` 抛 `std::out_of_range`；超出最大容量抛 `std::length_error`。

构建入口：

```powershell
cmake -S C06_Ranges\exercises\L05_dynamic_array -B C06_Ranges\exercises\L05_dynamic_array\build-author
cmake --build C06_Ranges\exercises\L05_dynamic_array\build-author --config Release --parallel 2
ctest --test-dir C06_Ranges\exercises\L05_dynamic_array\build-author -C Release --output-on-failure
```
## IDE 项目

生成 Visual Studio 工程后，启动项目是 `L05_dynamic_array_student`。

本题是实现题。学习者只改 Student 入口；Reference、validation 和 checks 只用于对照与验证。
- Student 入口：`src/student/dynamic_array.hpp`。
- Checker 入口：`main.cpp`。
- Reference 对照：`src/reference/dynamic_array.hpp`。
- validation/good 对照：`validation/good/dynamic_array.hpp`。
- validation/bad 反例：`validation/bad/dynamic_array.hpp`。

单题独立构建：从本目录运行 cmake -S . -B build/leaf -G "Visual Studio 18 2026" -A x64，然后 cmake --build build/leaf --config Debug --target L05_dynamic_array_student。
修改后先重建 `L05_dynamic_array_student`，再按课程 `BUILD_GUIDE.md` 运行对应检查。
