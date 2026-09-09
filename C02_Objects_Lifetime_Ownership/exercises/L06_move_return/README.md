# 练习 L06：移动与返回观察

先读 [06. 移动与返回](../../chapters/06-move-and-return.md)。本题是观察型练习，不提供 Student。

| Part | 操作 | 验证 |
|---|---|---|
| L06-1 | 观察 `return T{}` | copy/move 计数均为 0，说明同类型 prvalue 必然消除 |
| L06-2 | 观察 `return local` | 不断言 NRVO 一定发生，只允许 0 或 1 次 move |
| L06-3 | 观察 `return std::move(local)` | 当前自定义类型路径产生一次 move |
| L06-4 | 观察 `noexcept` move 对容器迁移的前提 | 自定义 nothrow move 类型可迁移；不从 trait 推断真实调用 |
| L06-5 | 编译 negative | named local 删除 copy/move 必须拒绝；`std::move(const move_only)` 必须拒绝 |

运行：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L06_move_return -B C02_Objects_Lifetime_Ownership/exercises/L06_move_return/build/c02-foundation-author -G "Visual Studio 18 2026" -A x64
cmake --build C02_Objects_Lifetime_Ownership/exercises/L06_move_return/build/c02-foundation-author --config Debug
ctest --test-dir C02_Objects_Lifetime_Ownership/exercises/L06_move_return/build/c02-foundation-author -C Debug --output-on-failure
```

解析：`return T{}` 是 C++17 起的同类型 prvalue 必然消除；`return local` 的 NRVO 是可选优化，程序仍需要可用 copy/move。`std::move` 只转值类别，`const` 对象移动会得到 `const T&&`，不能调用通常的 `T(T&&)`。本题只用自定义 `Tracked` 计数观察真实构造调用，不用 type trait 或标准库 moved-from 状态代替行为证据。

