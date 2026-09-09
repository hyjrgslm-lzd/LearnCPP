# 练习 L03：安全借用模型

先读 [03. 生命周期与借用](../../chapters/03-lifetime-and-borrowing.md)。本题是实现题，学生只编辑 `src/student/borrowed_int.hpp` 和 `src/student/borrowed_int.cpp`。

| Part | 操作 | 检查 |
|---|---|---|
| L03-1 | 实现 `BorrowedInt(l03_support::Owner&)` | 保存 owner 的 slot 和 generation，不复制值 |
| L03-2 | 实现 `is_valid()` 与 `get()` | owner 活着时读到最新值；owner 失效后报告 invalid，`get()` 抛模型异常 |
| L03-3 | 实现 `make_checked_reader(owner)` | lambda 捕获 borrow，不拥有 owner；owner 失效后返回 -1 |

checker 自己持有 `l03_support::Owner` 和 lifetime slot。Student 只能消费 fixture，不能返回手写报告。

运行：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L03_lifetimes -B C02_Objects_Lifetime_Ownership/exercises/L03_lifetimes/build/c02-foundation-author -G "Visual Studio 18 2026" -A x64
cmake --build C02_Objects_Lifetime_Ownership/exercises/L03_lifetimes/build/c02-foundation-author --config Debug
ctest --test-dir C02_Objects_Lifetime_Ownership/exercises/L03_lifetimes/build/c02-foundation-author -C Debug --output-on-failure
```

学生测试：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L03_lifetimes -B C02_Objects_Lifetime_Ownership/exercises/L03_lifetimes/build/student -G "Visual Studio 18 2026" -A x64 -DCORE_STUDY_TEST_STUDENTS=ON
cmake --build C02_Objects_Lifetime_Ownership/exercises/L03_lifetimes/build/student --config Debug
ctest --test-dir C02_Objects_Lifetime_Ownership/exercises/L03_lifetimes/build/student -C Debug --output-on-failure
```

解析：正确 borrow 保存身份和 generation，每次读取都回到 checker 的 owner slot。它不会缓存值，因此 owner 修改后能读到新值；owner 失效后也不会把旧值伪装成仍然有效。bad_dangling_view 故意只保存 slot 并忽略 generation/alive，用 checker 提供的 unsafe peek 读取旧槽位；它不会运行真实悬空指针，但会复现“owner 已结束而 borrower 仍声称可读”的错误，必须被拒绝。

