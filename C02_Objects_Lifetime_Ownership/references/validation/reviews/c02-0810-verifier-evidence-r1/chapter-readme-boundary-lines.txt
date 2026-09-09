Core_Study/exercises/L09_shared/README.md:3:先阅读 [09 shared_ptr：控制块、别名、weak_ptr 与循环](../../chapters/09-shared-ownership.md)。本题只编辑 `src/student/owner.hpp`。Reference 在 `src/reference/owner.hpp`，公共检查在 `checks/owner_checks.hpp`，受信资源模型在 `checks/support/shared_support.hpp`。
Core_Study/exercises/L09_shared/README.md:5:checker 先用相对路径包含真实 fixture，再用 `<owner.hpp>` 消费当前实现。Student 不能自带计数、报告或 ready 标记。
Core_Study/exercises/L09_shared/README.md:11:解析：checker 通过 `use_count()` 和 fixture alive 计数同时验证。只返回空指针或伪造结果会失败。
Core_Study/exercises/L10_control_block/README.md:1:# 练习 L10：单线程 rc_ptr / weak_rc 控制块模型
Core_Study/exercises/L10_control_block/README.md:3:先阅读 [10 控制块模型：实现单线程 rc_ptr 与 weak_rc](../../chapters/10-control-block.md)。本题只编辑 `src/student/rc.hpp`。Reference 在 `src/reference/rc.hpp`，公共检查在 `checks/rc_checks.hpp`，受信资源模型在 `checks/support/rc_support.hpp`。
Core_Study/exercises/L10_control_block/README.md:5:checker 先用相对路径包含真实 fixture，再通过 `<rc.hpp>` 消费当前实现。Student 不提供计数、报告或 ready 标记。本题模型是单线程教学模型，不要求原子计数、aliasing、删除器、分配器或 `enable_shared_from_this`。
Core_Study/exercises/L10_control_block/README.md:43:`validation/bad_leak_control_block` 会漏释放控制块。`validation/bad_weak_resurrect` 会在 weak 存在时保留对象或允许错误 lock。checker 用受信 fixture 的对象/控制块计数拒绝它们。
Core_Study/chapters/10-control-block.md:1:# 10 控制块模型：实现单线程 rc_ptr 与 weak_rc
Core_Study/chapters/10-control-block.md:3:L09 从使用者角度解释 `shared_ptr`。L10 写一个小型单线程模型，把控制块的生命周期完整拆开。目标不是复刻标准库，而是让 strong、weak、对象析构、控制块释放的因果关系变成可调试的代码。
Core_Study/chapters/10-control-block.md:5:本章类型名为 `rc_ptr<T>` 与 `weak_rc<T>`。它们只支持单线程；不支持 aliasing constructor；不支持 `enable_shared_from_this`；不提供自定义删除器和分配器。边界写清楚，是为了避免把教学模型误认为标准库替代品。
Core_Study/chapters/10-control-block.md:45:不能先把“对象 alive”或“控制块 alive”写进全局状态，再执行可能分配或抛异常的日志操作。本批次 fixture 的日志和计数使用固定状态，析构和 release 不分配。
Core_Study/chapters/10-control-block.md:80:Part 5：复验错误变体。`bad_leak_control_block` 漏释放控制块；`bad_weak_resurrect` 在 weak 存在时保留对象或允许过期 weak 变回 strong。checker 通过受信 fixture 的对象/控制块计数拒绝这些实现。
Core_Study/chapters/10-control-block.md:82:Student 只编辑 `src/student/rc.hpp`。公共检查先从相对路径加载 `checks/support/rc_support.hpp`，再包含 `<rc.hpp>`。实现头若需要 fixture，也应使用相对路径指向真实 support，不依赖可被 validation 目录遮蔽的搜索顺序。
Core_Study/chapters/08-unique-ownership.md:113:对本章练习，失败路径是 `Tracked` 构造函数按指定值抛异常。正确实现必须传播异常，同时保证没有 `Tracked` 存活、没有删除器事件伪造、没有 Student 自己提供计数事实。
Core_Study/chapters/08-unique-ownership.md:131:Part 3 复现构造失败：fixture 让指定 value 的 `Tracked` 构造抛异常。正确实现不能产生 owner，也不能遗留 alive 计数。
Core_Study/chapters/08-unique-ownership.md:137:本章通过 `checks/support/unique_support.hpp` 掌握真实构造、析构和删除器计数。Student 只能实现 owner 操作，不能提供完成标记或伪造报告。
Core_Study/exercises/L08_unique/README.md:3:先阅读 [08 unique_ptr：独占所有权、删除器与异常边界](../../chapters/08-unique-ownership.md)。本题只编辑 `src/student/owner.hpp`。Reference 在 `src/reference/owner.hpp`，公共检查在 `checks/owner_checks.hpp`，受信资源模型在 `checks/support/unique_support.hpp`。
Core_Study/exercises/L08_unique/README.md:5:checker 会先通过相对路径包含真实 fixture，再通过 `#include <owner.hpp>` 消费当前 target 的 include 路径。实现头若需要 fixture，也使用相对路径指向 `../../checks/support/unique_support.hpp`。Student 不提供计数、报告或 ready 标记。
Core_Study/exercises/L08_unique/README.md:29:fixture 可让特定 value 的 `Tracked` 构造抛异常。`make_tracked` 应传播异常，并且失败后没有 alive 对象。
Core_Study/exercises/L08_unique/README.md:47:解析：这是 pImpl 的最小模型。默认析构若放在只看得到前置声明的位置会失败；本练习把类型放在 fixture 中，重点验证独占成员和 move 后源为空。
Core_Study/chapters/09-shared-ownership.md:90:Part 1：构造 `Node` 并复制 `shared_ptr`，验证 `use_count` 增减和最后析构。fixture 只记录真实 `Node` 构造/析构。
Core_Study/chapters/09-shared-ownership.md:100:本章 checker 固定从 `checks/support/shared_support.hpp` 读取受信 fixture，再通过 `<owner.hpp>` 消费当前目标实现。Student 不能通过自带计数或 ready 标记绕过契约。
