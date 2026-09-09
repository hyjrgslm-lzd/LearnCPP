# 练习 L12：有界存储槽

先阅读 [12 存储与对象创建](../../chapters/12-storage-and-object-creation.md)。

你只编辑 `src/student/storage_slot.hpp`。实现 `l12::storage_slot<T>`：

- 默认空，析构时销毁仍活跃的对象。
- `construct(args...)` 在空槽中用 `std::construct_at` 创建对象并返回 `T&`。
- 已占用时再次 `construct` 抛 `std::logic_error`。
- `destroy()` 结束当前对象生命期；空槽调用无效果。
- `get()` 返回活跃对象引用；空槽访问抛 `std::logic_error`。
- `engaged()` 报告是否有活跃对象。

关键顺序：构造成功后才能把 `engaged` 设为 true。构造抛异常时槽仍为空，不能在失败清理中销毁未构造对象。

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L12_storage -B build/c02-l12 -G "Visual Studio 18 2026" -A x64 -DCORE_STUDY_TEST_STUDENTS=ON
cmake --build build/c02-l12 --config Debug
ctest --test-dir build/c02-l12 -C Debug --output-on-failure
```

解析：普通 raw storage 不会调用 `T` 构造函数；`allocator<T>::allocate(n)` 建立的是 `T[n]` 数组对象边界，不表示元素对象已经构造。元素生命期仍由 `std::construct_at` / `std::destroy_at` 管理。本题用 checker 中的受信计数器验证活跃对象数量，Student 不能通过填写报告或完成标记绕过。

本练习还提供 `L12_storage_start_lifetime_as` capability target。它只检测 C++23 `std::start_lifetime_as*`：

- 若 `__cpp_lib_start_lifetime_as` 缺失，测试输出 `SKIP: __cpp_lib_start_lifetime_as not defined`。
- 若宏存在，代码会真实实例化并运行 `std::start_lifetime_as<std::uint32_t>` 与 `std::start_lifetime_as_array<std::uint16_t>`。
- 不用 `construct_at` 或其他替代实现伪装为 capability PASS。

注意口径：普通 allocation 不调用构造函数；C++20 起某些操作可为 implicit-lifetime 类型隐式开始对象生命期；`std::allocator<T>::allocate(n)` 开始的是 `T[n]` 数组对象边界，不是 `n` 个元素对象生命期。
