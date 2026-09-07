# 源码导读 01：从 C++ 接口走到原子与 SIMD 后端

看到 `atomic<int>::fetch_add`，我们知道语言要求一次原子读改写；看到 `simd` 或 `batch`，我们知道接口描述一组元素的运算。实现还需要回答另外的问题：是哪一层选择机器操作？没有合适指令时怎么办？对齐、掩码和失败语义在哪一层落实？本篇追两条完整路径，避免在标准库头文件里搜到一个 builtin 名字就停下。

这些版本是为可复验选择的源码快照，不声称上游最新版本。2026-09-08 只读核查，未构建 GCC、libatomic 或 xsimd，未生成本篇对应机器码。当前 C++26 规范状态请从[标准索引](../../references/standards-and-implementations.md)进入；下面 GCC 的 experimental SIMD 是该版本的 TS 实现，xsimd 是第三方库，均不直接等同于标准 `std::simd`。

| 快照 | 完整 commit | 本篇入口 |
|---|---|---|
| GCC `releases/gcc-14.2.0` | [`04696df09633baf97cdbbdd6e9929b9d472161d3`](https://github.com/gcc-mirror/gcc/tree/04696df09633baf97cdbbdd6e9929b9d472161d3) | libstdc++ 头、gcc 编译器展开、libatomic 回退、experimental SIMD |
| xsimd `13.2.0` | [`1f8dd9c8e162968d9b4ff0251c56d431b8777f36`](https://github.com/xtensor-stack/xsimd/tree/1f8dd9c8e162968d9b4ff0251c56d431b8777f36) | batch、SSE2 kernel、runtime dispatcher；与课程固定源码版本一致 |

## 1. 先分整数特化和通用 atomic<T>

在 [include/std/atomic](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/libstdc%2B%2B-v3/include/std/atomic#L827) 找 `atomic<int>`，会看到它继承 `__atomic_base<int>`。继续到 [bits/atomic_base.h](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/libstdc%2B%2B-v3/include/bits/atomic_base.h#L629)，fetch_add 把 `_M_i` 的地址、增量和 `int(memory_order)` 交给 `__atomic_fetch_add`。这时 C++ 成员函数的参数已经转换成编译器内建调用，尚不能断言是一条特定机器指令。

通用 `atomic<T>` 不是先在对象中嵌入一把 mutex 再根据 sizeof 选择是否使用。这个提交首先存放按规则对齐的 `_M_i`，检查 T 的相应类型要求，再将 load/store/exchange 等交给 generic `__atomic_*`。通用 load 使用结果暂存区域接收内建写回的值；is_lock_free 和 is_always_lock_free 调用对应查询内建。锁回退可能存在于运行时库，而不是 atomic<T> 对象内部的一把可见 C++ mutex。[通用模板](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/libstdc%2B%2B-v3/include/std/atomic#L196)

因此旧文“大对象 atomic 内部其实有锁”应改成条件化的路径描述：大小、对齐、目标能力和构建配置共同决定实现；无法直接提供所需原子操作时，可能调用含锁的运行时回退。不能只凭一个 `sizeof(T)>8` 的判断跨架构宣布有锁。

## 2. 进入编译器：内建调用并没有消除目标选择

在 [gcc/builtins.cc](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/gcc/builtins.cc#L6895) 追 `BUILT_IN_ATOMIC_FETCH_ADD_*` 到 `expand_builtin_atomic_fetch_op`。它解析内存模型、地址和操作数，再尝试 `expand_atomic_fetch_op`。继续到 [gcc/optabs.cc](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/gcc/optabs.cc#L7812)，才会看到目标操作选择、变换、CAS 循环与回退条件。

该函数先检查所需大小的 load 能力，再尝试直接的目标展开；在适合的情况下也尝试等价运算、兼容的运行时调用或 compare-and-swap 循环。返回空的内部结果表示这一展开途径没有生成可用代码，编译器应继续相应回退处理，并不是用户 fetch_add 返回了一个“失败值”。`expand_atomic_load` 同样先查询目标 optab；无法按目标规则直接展开时保留库调用，或生成满足条件的普通 load 加屏障。[load 展开](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/gcc/optabs.cc#L7385)

这个层次划分也修正了旧文 `__atomic_* / __sync_*` 的混写。所追 libstdc++ 成员直接调用的是 `__atomic_*`；编译器下层可有兼容 `__sync` 的回退逻辑，但不能说 std::atomic 的头文件按类型大小随意在两套语义中选一个。GCC 的 [__atomic 内建官方说明](https://gcc.gnu.org/onlinedocs/gcc-14.2.0/gcc/_005f_005fatomic-Builtins.html)明确给出参数和运行时支持背景，最终仍要以这个 commit 的实际路径核对。

读到这里可以定位“在哪层继续追 x86 或 AArch64”，但还不能给出本次生成的 XADD、LDADD 或 LL/SC 序列。那需要目标后端条件、编译选项与汇编产物。本篇没有生成汇编，机器指令名称只可作为后续搜索问题，不作为实测结论。课程已有的[原子操作正文](../atomics/01-atomic-operations.md)与[内存模型](../../chapters/09-memory-model.md)仍负责语言层合同。

## 3. CAS 的两个出口：更新对象，或回填 expected

整数 `compare_exchange_weak` 把 expected 的地址交给 `__atomic_compare_exchange_n`，weak 参数为 1；strong 版本为 0。双内存序重载分别传 success 和 failure；单内存序重载通过 `__cmpexch_failure_order` 将 acq_rel 对应到 acquire，将 release 对应到 relaxed。失败是读取并回填观察值，不执行成功写入的 release 行为。[CAS 包装](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/libstdc%2B%2B-v3/include/bits/atomic_base.h#L530)

阅读时把 expected 看成调用方对象，不能把失败当作“不发生任何写入”：共享 atomic 可能没被该次 CAS 修改，但 expected 已有新值。weak 还允许相应伪失败，不能从一次 false 推出比较值一定不同。课程 [CAS 练习推导](../atomics/02-cas-and-minmax.md)中的循环，正是利用这个失败出口更新下一次尝试。

源码中的 `__glibcxx_assert` 是实现诊断，不替应用放宽 memory_order 的接口前提。断言在某个构建中不启用，也不能把非法 failure order 变成有效用法。泛型 atomic 的 padding 处理另有 helper；不要把 libatomic 底层的一次 memcmp 直接解释为所有 C++ atomic<T> 的完整值比较规则。

## 4. libatomic 回退：原子性也可以由外部锁表实现

从 generic load 继续读 [libatomic/gload.c](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/libatomic/gload.c#L67) 的 libat_load。它先尝试适合大小与对齐的专门路径，或覆盖目标字节的更宽操作；这些条件不成立时，执行序前处理、libat_lock_n、复制结果、libat_unlock_n、序后处理。调用者得到完整值，但这条分支的实现可以等待锁。

[libatomic/gcas.c](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/libatomic/gcas.c#L78) 的 generic CAS 最后也有锁回退：比较成功时将 desired 复制到对象，失败时把对象当前字节复制到 expected，随后解锁并返回相应布尔值。这里仍实现两个不同出口，锁没有让 CAS 失败语义消失。

这把锁在哪里？本篇选 [libatomic/config/posix/lock.c](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/libatomic/config/posix/lock.c) 的 POSIX 配置作明确例子。它维护静态带 padding 的 pthread mutex 数组，通过 addr_hash 将地址映射到锁；覆盖多段地址范围的操作调用 libat_lock_n/unlock_n。两个不同 atomic 对象可能映射到相同锁位置，从而共享竞争成本，锁并非必然嵌在 T 的旁边。

这是一个选定平台配置的源码说明，不能把 pthread 路径说成本机 Windows 已运行的行为。查 is_lock_free、目标能力、实际链接库与机器码，才可以对某个部署二进制的进展下结论。此处 atomic 不拥有外部指针指向的对象，也没有 HP 式回收出口；它的成功/失败只规定值操作，指针生命期仍由应用协议负责。

## 5. GCC experimental SIMD：ABI 标签选择存储与操作后端

旧 18 的另一条 GCC 路线位于 [experimental/bits/simd.h](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/libstdc%2B%2B-v3/include/experimental/bits/simd.h#L2991)。先追 `simd_abi::native<T>` 到 `__determine_native_abi`，它根据编译期目标能力与向量字节数选择 scalar、SVE、带位掩码的 ABI 或 `_VecBuiltin`。这里主要是编译期选择，不是在每次 load 时检查运行机器。

再到 [simd_builtin.h](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/libstdc%2B%2B-v3/include/experimental/bits/simd_builtin.h#L814)：`_VecBuiltin<UsedBytes>` 给出元素数、有效性约束及 `_SimdImpl`/`_MaskImpl` 的后端类型。条件编译将它们接到 X86、Neon、PPC 或 builtin 实现；x86 的具体扩展在 [simd_x86.h](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/libstdc%2B%2B-v3/include/experimental/bits/simd_x86.h#L876)。所以一个 ABI 标签影响的不只是类型名字，还包括寄存器表示、掩码表示及可用操作。

成功 load 的路径是 simd 构造或 copy_from → `_Flags::_S_apply` 处理地址的对齐约定 → `_Impl::_S_load` → 写入 `_M_data`；copy_to 则从 `_M_data` 经 `_S_store` 写回调用者缓冲区。向量对象拥有加载后的值，不拥有原输入缓冲区；后续 store 的目标范围仍由调用者保证。[加载与存储入口](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/libstdc%2B%2B-v3/include/experimental/bits/simd.h#L5497)

`where(mask, value)` 不是一份独立拥有数据的容器。沿 where_expression 的赋值走到 `_S_masked_assign`，会看到按掩码更新被引用向量的元素；未选中部分保持原值。其保存的引用不能超过对应对象寿命。掩码选择也不自动把一个越界的完整向量 load 变成安全尾部访问：先整批读取再 select 时，非法读取已经发生。应追具体 masked load/copy_from 重载，而不是仅寻找一个 where 字样。[where 赋值](https://github.com/gcc-mirror/gcc/blob/04696df09633baf97cdbbdd6e9929b9d472161d3/libstdc%2B%2B-v3/include/experimental/bits/simd.h#L3620)

无效 ABI/类型组合可能在模板约束阶段被拒绝；错误对齐或越界输入通常是违反调用前提，不是返回 false 的正常分支。TS 与 N5050 标准接口的名称、掩码类型和加载形式需分别对照标准索引，不能把这份 experimental 头的存在视为 C++26 实现能力探测通过。

## 6. xsimd：从 batch 到 SSE2 kernel，再区分运行时分派

选择课程已使用的 `batch<float, sse2>`，先看 [xsimd_batch.hpp](https://github.com/xtensor-stack/xsimd/blob/1f8dd9c8e162968d9b4ff0251c56d431b8777f36/include/xsimd/types/xsimd_batch.hpp#L113)。batch 继承相应 simd_register，size 来自该寄存器表示与标量大小；batch_bool 另有掩码表示。`load_aligned` 调 kernel::load_aligned，unaligned 走另一重载；SSE2 float 实现分别调用 `_mm_load_ps` 与 `_mm_loadu_ps`。[SSE2 加载](https://github.com/xtensor-stack/xsimd/blob/1f8dd9c8e162968d9b4ff0251c56d431b8777f36/include/xsimd/arch/xsimd_sse2.hpp#L872)

沿 [xsimd_api.hpp](https://github.com/xtensor-stack/xsimd/blob/1f8dd9c8e162968d9b4ff0251c56d431b8777f36/include/xsimd/types/xsimd_api.hpp#L1831) 的 reduce_add 与 select 继续到 SSE2 kernel：水平求和将多个通道组合成标量；select 对两个已形成的向量值做按掩码选择。前者改变浮点结合方式，后者不是惰性执行两个业务分支。和 [课程数值精度](../simd/03-reductions-and-precision.md)、[显式加载与尾部](../simd/02-explicit-vectors.md)对照时，应保留原有误差和访问范围检查，不能因用了第三方库就删掉。

固定一个 Arch 类型与运行时 dispatch 是两层机制。`dispatch(functor)` 构造 dispatcher，保存 available_architectures 的检测结果和 functor；调用时 walk_archs 沿架构列表选择，向 functor 传入被选中的 Arch 标签，再由该类型实例化相应内核。[dispatcher](https://github.com/xtensor-stack/xsimd/blob/1f8dd9c8e162968d9b4ff0251c56d431b8777f36/include/xsimd/config/xsimd_arch.hpp#L192)

`xsimd_cpuid.hpp` 中可继续核对 x86 的 CPUID、OSXSAVE 与 XGETBV 分支以及最终各能力字段的赋值。本篇确认的是这个检测实现的控制流，不宣称已穷尽各种 OS/虚拟化组合的检测正确性。最后一个架构候选分支会直接调用 functor，调用者仍需保证候选集合含可执行基线；dispatch 不会替你编译一个不存在的 ISA 版本，也不会把整个程序按高级 ISA 编译后自动隔离早期指令。[能力检测](https://github.com/xtensor-stack/xsimd/blob/1f8dd9c8e162968d9b4ff0251c56d431b8777f36/include/xsimd/config/xsimd_cpuid.hpp#L124)

batch 值析构没有独立堆内存回收任务，源/目的缓冲区的所有权和寿命都在调用方。错误地址不是受支持的失败通道；dispatcher 的 noexcept 调用路径也不是通用的业务异常传送器。先固定输入与不抛出内核，再研究实际分派，不要让另一个未声明的异常契约混入阅读任务。

## 7. 逐项对照与可复验任务

| 路线 | 对照课程 | 必须保留的边界 |
|---|---|---|
| atomic 成员→内建→目标展开→运行时 | 原子、CAS、内存模型 | 返回语义不由机器指令名字决定；完整操作是否无锁要验证目标 |
| libatomic 外部锁表 | 锁与伪共享 | atomic 对象不一定内嵌 mutex，锁竞争可来自地址散列 |
| SIMD ABI→向量表示→加载/掩码 | 标量基线、显式 SIMD | 编译期 ABI、访问前提、数值结合顺序不相互替代 |
| xsimd Arch 与 dispatch | ISA 与分派诊断 | 编译得到的版本、运行能力、基线安全三者都要成立 |

本地已有固定检出时，先核对 SHA，按以下符号跳转；没有检出直接用固定网页，不新增编译或运行依赖：

```powershell
git -C $src rev-parse HEAD
rg -n 'struct atomic<int>|__atomic_always_lock_free|__atomic_load' "$src/libstdc++-v3/include/std/atomic"
rg -n '__atomic_fetch_add|__atomic_compare_exchange_n|__cmpexch_failure_order' "$src/libstdc++-v3/include/bits/atomic_base.h"
rg -n 'expand_builtin_atomic_fetch_op|expand_builtin_atomic_load' "$src/gcc/builtins.cc"
rg -n 'expand_atomic_fetch_op|expand_atomic_load' "$src/gcc/optabs.cc"
rg -n 'libat_load|libat_lock_n|memcpy' "$src/libatomic/gload.c" "$src/libatomic/config/posix/lock.c"
rg -n '__determine_native_abi|where_expression|_S_load|copy_to' "$src/libstdc++-v3/include/experimental/bits/simd.h"
# 将 src 改为本篇 xsimd 固定检出
rg -n 'load_aligned|load_unaligned|reduce_add|select' "$src/include/xsimd/arch/xsimd_sse2.hpp"
rg -n 'dispatcher|walk_archs|available_architectures' "$src/include/xsimd/config/xsimd_arch.hpp"
```

**任务 A：为 fetch_add 画出四层调用链，在哪层才能研究目标指令？** 答案：atomic<int> → __atomic_base<int>::fetch_add → __atomic_fetch_add → builtins/optabs 展开与目标处理；头文件只是入口。最终机器码需要实际编译证据，本篇没有这项产物。

**任务 B：用一张表写 CAS 成功与失败改变了谁。** 答案：成功把 desired 写入 atomic，返回 true；失败返回 false 并更新 expected 为相应观察值，weak 允许伪失败。单序重载还派生合法 failure order；libatomic 锁回退仍保留这两个出口。

**任务 C：找到一条真正含锁的 atomic 路径，不得凭 sizeof 猜。** 答案：generic libat_load 的专门路径全部不适用时，进入 libat_lock_n；所选 POSIX 配置通过地址散列访问 pthread mutex 数组。说明这是条件路径和指定配置，不能说当前 MSVC 已走过它。

**任务 D：native SIMD 的“native”是运行时自动选最宽吗？** 答案：该 GCC TS 路径主要由编译期能力和类型决定 ABI；xsimd runtime dispatcher 是额外层。两者都不能凭名字保证某个部署机器安全或每次用最宽 ISA。

**任务 E：尾部只有三个 float，能先 load 四个再 select 掉第四个吗？** 答案：若第四个不在合法访问范围，不能。选择发生在加载以后，不能撤销越界；应使用经过核对的安全尾部或 masked load 方式。查函数体中的真实 load 位置，再与课程尾部协议对照。

**任务 F：读取 batch 后原数组销毁，batch 是否悬垂？where 表达式呢？** 答案：普通 batch/simd 保存已加载的向量值，不继续借用输入数组；where 表达式可能保存目标/掩码引用，寿命义务另算。store 仍要求输出指针有效，值拥有与外部缓冲区拥有不能混同。

旧 18 的 atomic、GCC experimental SIMD、xsimd 类型/掩码/对齐/分派路线在此保留。核查深度是源码路径与契约，不是本篇重新实现编译器，也不是宣称所有生产库都处在最新发布版本。
