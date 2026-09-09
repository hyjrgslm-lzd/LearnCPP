# 练习 L07：两个资源的 RAII 与部分构造失败

先阅读 [07 RAII 与所有权](../../chapters/07-raii-and-ownership.md)。本题只验证一个问题：两个资源顺序获取时，第二个资源获取失败，第一个资源是否仍能被释放。

你只编辑 `src/student/owner.hpp`。Reference 在 `src/reference/owner.hpp`，公共检查在 `checks/owner_checks.hpp`，安全资源模型在 `checks/support/resource_model.hpp`。checker 会先用相对路径包含真实 fixture，再通过 `#include <owner.hpp>` 消费当前目标的 include 路径；正常实现也应使用相对路径包含这个 fixture。资源计数、事件和失败注入只由公共 fixture 掌握，Student 不能通过伪造 `resource_model.hpp` 冒充完成。

## Part 1：观察手工 acquire/release 的成功路径

运行 observation 目标。它先执行手工基线：

```cpp
auto first = acquire_first();
auto second = acquire_second();
release_second(second);
release_first(first);
```

成功路径应看到资源归零，事件顺序是 first acquire、second acquire、second release、first release。这个基线不是错代码；它只是只覆盖了没有异常的路径。

解析：两个资源都成功获取后，后续 release 语句能正常执行。释放顺序选择反向，是为了让后获取的资源先断开依赖。此时没有构造失败，也没有跳过清理语句。

## Part 2：复现新的失败路径

observation 接着把失败点放到第二个 acquire。手工代码在 `acquire_second()` 抛异常后直接离开作用域，后面的 release 语句没有执行。

预期观察：`first_alive == 1`，`second_alive == 0`。这证明模型里第一个资源遗留。它不证明真实 UB，也不依赖崩溃。

解析：`first` 是一个普通整数。异常展开会销毁 C++ 对象，但销毁 `int` 不会调用 `release_first`。清理动作没有绑定到已构造对象，就不会自动发生。

## Part 3：解释构造失败规则

把两个 acquire 放进外层对象构造函数后，如果第二个 acquire 抛异常，外层对象没有完成构造。C++ 不调用外层析构函数，只析构已经构造完成的基类和成员。

你需要在答案里写出两个判断：

- 外层析构函数没有运行，所以不能把“最终清理”只写在那里。
- 已构造成员会被析构，所以每个资源应尽早进入自己的 RAII 成员。

解析：这是 RAII 能修复部分构造失败的语言基础。构造函数体里的本地变量和普通成员值不会表达资源所有权；带析构责任的成员对象才表达。

## Part 4：实现 `two_resource_owner`

在 `src/student/owner.hpp` 中实现与 Reference 相同的公开接口。资源 API 来自 `l07_support`：

- `l07_support::acquire_first()` 获取 first 资源。
- `l07_support::acquire_second()` 获取 second 资源。
- `l07_support::release_first(handle)` 释放 first 资源。
- `l07_support::release_second(handle)` 释放 second 资源。
- `two_resource_owner` 默认构造时获取 first 再获取 second。
- `two_resource_owner` 析构时释放仍拥有的资源，释放顺序为 second 后 first。
- 禁止复制，允许 noexcept move。
- `reset()` 可重复调用，第二次应无效果。

解析：实现可以手写两个小 RAII 成员，也可以直接在 `two_resource_owner` 内维护两个 id，但必须覆盖第二个 acquire 抛异常时释放第一个资源。最稳的做法是让 first 资源先进入已经构造完成的成员，再获取 second。

## Part 5：复验原失败点与所有权转交

完成后打开学生测试：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L07_raii -B build/c02-owner-author -G "Visual Studio 18 2026" -A x64 -DCORE_STUDY_TEST_STUDENTS=ON
cmake --build build/c02-owner-author --config Debug
ctest --test-dir build/c02-owner-author -C Debug --output-on-failure
```

检查器会覆盖：

- 正常构造和作用域退出后资源归零。
- 第一个 acquire 抛异常后没有资源遗留。
- 第二个 acquire 抛异常后 first 被释放。
- 多个 owner 交错存活时，reset 一个 owner 不影响另一个 owner。
- move 构造后源对象为空，目标对象拥有两个资源。
- move 赋值会先释放目标旧资源，再接管源资源。
- self move 不丢资源。
- `reset()` 幂等。
- 类型不可复制，move 构造和 move 赋值为 `noexcept`。

解析：这些检查对应独占所有权的基本不变量。资源归零证明当前路径释放匹配；源对象为空证明 move 是转交，不是复制；不可复制避免两个 owner 同时释放同一个资源。

## 公开验证变体

`validation/good/owner.hpp` 复用 Reference，代表独立完成体，应通过同一份契约 checker。`validation/bad_noop/owner.hpp` 是假完成但完全不获取资源的实现，`validation/bad_fake_completed/owner.hpp` 会获取资源但漏释放 second，`validation/bad_memberwise_move_order/owner.hpp` 使用默认 memberwise move assignment，释放旧 target 时顺序错误，`validation/bad_support_shadow` 尝试用本地 `resource_model.hpp` 遮蔽受信 fixture。它们用于证明 checker 实际消费当前 `owner.hpp`，并且能拒绝代表性错误操作和 fixture shadow。

需要验证 checker 自身时，打开本题的本地验证变体：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L07_raii -B build/c02-owner-author-validation -G "Visual Studio 18 2026" -A x64 -DL07_RAII_BUILD_VALIDATION_VARIANTS=ON
cmake --build build/c02-owner-author-validation --config Debug --target L07_raii_validation_good L07_raii_validation_bad_noop L07_raii_validation_bad_fake_completed L07_raii_validation_bad_memberwise_move_order
./build/c02-owner-author-validation/Debug/L07_raii_validation_good.exe
./build/c02-owner-author-validation/Debug/L07_raii_validation_bad_noop.exe
./build/c02-owner-author-validation/Debug/L07_raii_validation_bad_fake_completed.exe
./build/c02-owner-author-validation/Debug/L07_raii_validation_bad_memberwise_move_order.exe
cmake --build build/c02-owner-author-validation --config Debug --target L07_raii_validation_bad_support_shadow
```

Reference 最后输出 `L07_raii_reference OK`。good 变体最后输出 `L07_raii_validation_contract OK`。Student 占位、bad no-op 和 bad fake completed 都应非零退出，并输出具体 `check failed: ...` 诊断。
