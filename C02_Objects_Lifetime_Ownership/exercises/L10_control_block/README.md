# 练习 L10：单线程 rc_ptr / weak_rc 控制块模型

先阅读 [10 控制块模型：实现单线程 rc_ptr 与 weak_rc](../../chapters/10-control-block.md)。本题只编辑 `src/student/rc.hpp`。Reference 在 `src/reference/rc.hpp`，公共检查在 `checks/rc_checks.hpp`，受信资源模型在 `checks/support/rc_support.hpp`。

checker 先用相对路径包含真实 fixture，再通过 `<rc.hpp>` 消费当前实现。Student 不提供计数、报告或 ready 标记。本题模型是单线程教学模型，不要求原子计数、aliasing、删除器、分配器或 `enable_shared_from_this`。

## Part 1：控制块和 make_rc

实现 `make_rc<T>(args...)`，创建控制块并在其中构造 `T`。成功后 strong 为 1，用户 weak 为 0，对象和控制块都 alive。

解析：如果 `T` 构造抛异常，异常要传播，对象 alive 和控制块 alive 都必须回到 0。

## Part 2：rc_ptr 强引用

实现 `rc_ptr<T>`：

- 默认构造为空。
- copy 增加 strong。
- move 转移控制块指针并清空源。
- `reset()` 释放一个 strong。
- 最后 strong 释放时析构对象。
- 若没有用户 weak，同时释放控制块。

解析：self copy/move assignment 要保持计数稳定。move 不增加也不减少 strong，只改变哪个对象负责释放。还要处理 RHS 位于旧 pointee 内部的合法场景：`p = p->child` 应先 retain RHS 控制块再释放旧 LHS；`p = std::move(p->child)` 应先把 RHS 控制块交换到局部变量，再释放旧 LHS。

## Part 3：weak_rc 弱引用

实现 `weak_rc<T>` 从 `rc_ptr<T>` 构造、copy、move、reset 和析构。weak 不保持对象存活，但对象销毁后只要 weak 还在，控制块仍应存在。

解析：控制块生命周期长于对象生命周期，这是 shared/weak 两阶段模型的核心。

## Part 4：expired 与 lock

实现：

- `expired()`：没有控制块或 strong 为 0 时为真。
- `lock()`：strong 大于 0 时返回新的 `rc_ptr`，否则返回空。

解析：lock 不能把 strong 从 0 加回 1。对象析构后不可复活。

## Part 5：公开坏变体

`validation/bad_leak_control_block` 会漏释放控制块。`validation/bad_weak_resurrect` 会在 weak 存在时保留对象或允许错误 lock。checker 用受信 fixture 的对象/控制块计数拒绝它们。

## 验证

学生测试：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L10_control_block -B build/c02-owner-l10-student -G "Visual Studio 18 2026" -A x64 -DCORE_STUDY_TEST_STUDENTS=ON
cmake --build build/c02-owner-l10-student --config Debug
ctest --test-dir build/c02-owner-l10-student -C Debug --output-on-failure
```

public validation：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L10_control_block -B build/c02-owner-l10 -G "Visual Studio 18 2026" -A x64 -DL10_CONTROL_BLOCK_BUILD_VALIDATION_VARIANTS=ON
cmake --build build/c02-owner-l10 --config Debug
./build/c02-owner-l10/Debug/L10_control_block_validation_good.exe
./build/c02-owner-l10/Debug/L10_control_block_validation_bad_leak_control_block.exe
./build/c02-owner-l10/Debug/L10_control_block_validation_bad_weak_resurrect.exe
```

`validation/good` 应输出 `L10_control_block_validation_contract OK`。两个 bad 变体应非零退出并给出具体 `check failed: ...`。
