# 练习 L09：shared_ptr 控制块、aliasing 与 weak_ptr

先阅读 [09 shared_ptr：控制块、别名、weak_ptr 与循环](../../chapters/09-shared-ownership.md)。本题只编辑 `src/student/owner.hpp`。Reference 在 `src/reference/owner.hpp`，公共检查在 `checks/owner_checks.hpp`，受信资源模型在 `checks/support/shared_support.hpp`。

checker 先用相对路径包含真实 fixture，再用 `<owner.hpp>` 消费当前实现。Student 不能自带计数、报告或 ready 标记。

## Part 1：共享强所有权

实现 `make_node(int)` 返回 `std::shared_ptr<l09_support::Node>`。copy 应共享同一控制块，最后一个 strong 离开后节点析构。

解析：checker 通过 `use_count()` 和 fixture alive 计数同时验证。只返回空指针或伪造结果会失败。

## Part 2：stored pointer 与控制块分离

实现 `alias_value(const std::shared_ptr<Node>&)`，返回指向 `Node::value` 的 `std::shared_ptr<int>`，但共享原 `Node` 的控制块。

解析：正确写法使用 aliasing constructor：`std::shared_ptr<int>(owner, &owner->value)`。owner reset 后 alias 仍保持整个 Node 存活。

## Part 3：weak 观察和 lock

实现：

- `observe(const std::shared_ptr<node>& owner)` 返回 `std::weak_ptr<node>`。
- `lock_value(const std::weak_ptr<node>& weak, int& out)` 尝试 `weak.lock()`；成功时把节点 value 写入 `out` 并返回 `true`，失败时返回 `false`。

解析：对象销毁后 weak 过期，`lock()` 返回空。checker 在 strong 存活时要求 `lock_value` 返回 `true` 且写出真实 value；scope 结束后要求返回 `false`。bad 变体会在 expired 后假装 lock 成功，checker 必须拒绝。

## Part 4：循环修复

实现 `connect_parent_child(parent, child)`：把 checker 创建的真实父子节点连起来。父节点用 shared `next` 拥有子节点，子节点用 weak `parent` 观察父节点。函数不返回结果；checker 会直接检查节点成员、外部 weak 和 fixture alive 计数。

解析：反向边若用 shared，会形成 cycle。外部局部变量释放后，两个节点仍互相持有 strong，析构不会运行。正确实现用 weak 打断所有权环。

## Part 5：enable_shared_from_this

实现 `shared_from_existing(node&)`：对象已由 checker 持有的 `shared_ptr` 管理后，从这个真实对象调用 `shared_from_this()`，返回共享同一控制块的新 strong。函数不返回自报计数；checker 会直接比较地址和 `owner.use_count()`。

解析：不要用 `std::shared_ptr<Node>(this)`。本题不在默认路径制造双控制块 UB，只验证正确前提。

## 验证

学生测试：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L09_shared -B build/c02-owner-l09-student -G "Visual Studio 18 2026" -A x64 -DCORE_STUDY_TEST_STUDENTS=ON
cmake --build build/c02-owner-l09-student --config Debug
ctest --test-dir build/c02-owner-l09-student -C Debug --output-on-failure
```

public validation：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L09_shared -B build/c02-owner-l09 -G "Visual Studio 18 2026" -A x64 -DL09_SHARED_BUILD_VALIDATION_VARIANTS=ON
cmake --build build/c02-owner-l09 --config Debug
./build/c02-owner-l09/Debug/L09_shared_validation_good.exe
./build/c02-owner-l09/Debug/L09_shared_validation_bad_hardcoded_noop.exe
./build/c02-owner-l09/Debug/L09_shared_validation_bad_cycle.exe
./build/c02-owner-l09/Debug/L09_shared_validation_bad_weak_resurrect.exe
```

`validation/good` 应输出 `L09_shared_validation_contract OK`。bad hardcoded noop 应在真实节点连边检查处失败；bad cycle 应被 alive 计数拒绝；bad weak resurrect 应被 expired/lock 契约拒绝。
