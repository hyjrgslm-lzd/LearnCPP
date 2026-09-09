# 练习 L08：unique_ptr 独占所有权边界

先阅读 [08 unique_ptr：独占所有权、删除器与异常边界](../../chapters/08-unique-ownership.md)。本题只编辑 `src/student/owner.hpp`。Reference 在 `src/reference/owner.hpp`，公共检查在 `checks/owner_checks.hpp`，受信资源模型在 `checks/support/unique_support.hpp`。

checker 会先通过相对路径包含真实 fixture，再通过 `#include <owner.hpp>` 消费当前 target 的 include 路径。实现头若需要 fixture，也使用相对路径指向 `../../checks/support/unique_support.hpp`。Student 不提供计数、报告或 ready 标记。

## Part 1：单对象 owner 与删除器

实现：

- `using tracked_ptr = std::unique_ptr<l08_support::Tracked, l08_support::CountingDelete>`
- `make_tracked(int)` 返回拥有对象的 owner。
- `read_tracked(const tracked_ptr&)` 返回对象 value，空 owner 返回 `-1`。
- `borrow_raw(const tracked_ptr&)` 只观察地址，不转移所有权。

解析：`CountingDelete` 是所有权语义的一部分。checker 看真实构造、析构和 deleter 计数，拒绝不创建对象的假完成。

## Part 2：release 与 reset

实现：

- `release_tracked(tracked_ptr&)` 返回裸指针，并让 owner 为空。
- `reset_tracked(tracked_ptr&, int)` 释放旧对象后持有新对象。

解析：`release()` 是责任转移，不是借用。bad 变体会返回裸指针但保留 owner；checker 在删除裸指针前先验证 owner 为空，避免默认路径制造 double delete。

## Part 3：构造失败

fixture 可让特定 value 的 `Tracked` 构造抛异常。`make_tracked` 应传播异常，并且失败后没有 alive 对象。

解析：正确路径通常用 `std::unique_ptr` 或 `std::make_unique` 风格封装对象构造。构造失败时不能留下半完成 owner 或伪造计数。

## Part 4：数组 owner

实现：

- `using int_array = std::unique_ptr<int[]>`
- `make_array(int size, int first_value)` 创建长度为 `size` 的数组，并从 `first_value` 开始依次写入递增整数。
- `sum_array(const int_array& values, int size)` 按传入长度求和。

解析：`unique_ptr<T[]>` 使用 `delete[]`，但不保存长度。checker 调用 `make_array(4, 10)`，期望数组内容是 `10, 11, 12, 13`，因此 `sum_array(values, 4)` 应返回 `46`。这只验证所有权和索引访问，不把它当 `vector`。

## Part 5：不完整类型 owner

实现 `incomplete_owner`：移动专用、不可复制，`explicit incomplete_owner(int value)` 创建并拥有一个 `IncompleteState`，析构释放它，`value()` 返回当前状态值；移动后的源对象可为空，空状态返回 `-1`。

解析：这是 pImpl 的最小模型。默认析构若放在只看得到前置声明的位置会失败；本练习把类型放在 fixture 中，重点验证独占成员和 move 后源为空。

## 验证

学生测试：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L08_unique -B build/c02-owner-l08-student -G "Visual Studio 18 2026" -A x64 -DCORE_STUDY_TEST_STUDENTS=ON
cmake --build build/c02-owner-l08-student --config Debug
ctest --test-dir build/c02-owner-l08-student -C Debug --output-on-failure
```

public validation：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L08_unique -B build/c02-owner-l08 -G "Visual Studio 18 2026" -A x64 -DL08_UNIQUE_BUILD_VALIDATION_VARIANTS=ON
cmake --build build/c02-owner-l08 --config Debug
./build/c02-owner-l08/Debug/L08_unique_validation_good.exe
./build/c02-owner-l08/Debug/L08_unique_validation_bad_noop.exe
./build/c02-owner-l08/Debug/L08_unique_validation_bad_release_keeps_owner.exe
```

`validation/good` 应输出 `L08_unique_validation_contract OK`。两个 bad 变体应非零退出并给出 `check failed: ...`。
