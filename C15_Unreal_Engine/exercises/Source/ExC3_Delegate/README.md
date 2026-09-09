> 对应章节: ../../../04-模块C-内存-智能指针-委托.md §练习 C-3

## 目标

从零手写一个 `FMiniIntDelegate`（最小多播委托），支持 `AddLambda` / `Remove` / `Broadcast`；通过与 UE 源码宏展开物的对比表，理解工业级实现在哪些维度比 mini 版本复杂；明确 C++ delegate（本题）与 `DECLARE_DYNAMIC_MULTICAST_DELEGATE`（反射层）的边界。

## 前置理解

- 理解 C++ 函数指针与 `std::function` 的类型擦除机制；UE delegate 的核心也是类型擦除，但以句柄（Handle）管理绑定。
- 了解 `DECLARE_MULTICAST_DELEGATE_OneParam` 等宏的调用形态（本题的学习目标是理解其展开物）。
- 已读 `04-模块C` §C-3 前置理解部分，明确 `DECLARE_DYNAMIC_MULTICAST_DELEGATE*` 与 `DECLARE_MULTICAST_DELEGATE*` 的边界。

## 必做任务

1. 实现 `FMiniDelegateHandle`：包含 `uint64 ID`，`ID==0` 为无效句柄；静态计数器从 1 开始递增生成唯一 ID。
2. 定义 `FMiniBinding`：持有 `FMiniDelegateHandle` 和 `TFunction<void(int32)> Func`。
3. 实现 `FMiniIntDelegate::AddLambda`：生成句柄、创建绑定、追加到 `TArray<FMiniBinding>`，返回句柄。
4. 实现 `FMiniIntDelegate::Remove`：按 `Handle.ID` 线性查找并删除对应条目（幂等）。
5. 实现 `FMiniIntDelegate::Broadcast`：遍历 `Bindings`，逐一调用 `Func(Value)`（骨架阶段不处理 Remove-during-Broadcast）。
6. 验证：`Broadcast(42)` 触发 H1+H2；`Remove(H1)` 后 `Broadcast(7)` 仅触发 H2。
7. 填写 mini 实现与 UE 展开物的差异对比表（见本题教材，至少 4 行）。

## 进阶任务

- 实现 `FMiniSingleDelegate`（单播）：`BindLambda` / `Execute`（未绑定时 `check()` 失败）/ `ExecuteIfBound` / `IsBound`。
- 支持 payload：在 `AddLambda` 时捕获额外的 `float Payload`，使 `Broadcast(int32)` 签名不变而 payload 对调用方透明。
- 延迟删除防护：Broadcast 期间把要删除的句柄记录进 `PendingRemove`，Broadcast 结束后再真正移除。

## 验收点

- [ ] `AddLambda` / `Remove` / `Broadcast` 三个接口功能正确，验证代码跑通。
- [ ] `FMiniDelegateHandle` 的 ID 唯一性有保证（静态计数器，不复用已删除的 ID）。
- [ ] 对比表至少 4 行，每行内容准确。
- [ ] 能书面说明为什么不能把 `DYNAMIC_MULTICAST_DELEGATE` 视为本题实现的扩展版。
- [ ] 能答出四个固定问题。

## 观察点

- UE `TFunction`（`Function.h`）是 UE 版 `std::function`，支持 Small Buffer Optimization（小 functor 内联存储，默认 32 字节，超过则堆分配）。
- `DECLARE_MULTICAST_DELEGATE_OneParam(FMyDelegate, int32)` 展开后生成继承自 `TMulticastDelegate<void(int32)>` 的类；核心方法是 `AddLambda` / `AddUObject` / `AddStatic` / `AddWeakLambda` / `Remove` / `Broadcast`——你的 mini 版本只实现了其中的子集。
- `DECLARE_DYNAMIC_MULTICAST_DELEGATE*` 绑定的目标是 `UFunction`（UHT 生成的反射函数对象），绑定语法（`AddDynamic` 宏）完全不同，序列化到磁盘的内容是函数名字符串而不是函数指针。

## 常见坑

- **在 `Broadcast` 中途 `Remove`**：`TArray` 线性遍历中 `Remove` 会触发元素移位，产生跳过或重复调用。进阶任务的延迟删除是修复此问题的标准模式。
- **误以为 `DECLARE_DYNAMIC_MULTICAST_DELEGATE` 是 `DECLARE_MULTICAST_DELEGATE` 的超集**：两者不是继承关系；Dynamic 版本依赖 UHT 代码生成和 `UFunction` 反射对象，不应在无需蓝图可见性的场合滥用。
- **句柄 ID 用 `int32` 而非 `uint64`**：理论上存在 ID 复用问题，生产代码必须用 `uint64`。
- **lambda 捕获了 `UObject*` 但未使用 `TWeakObjectPtr`**：GC 可能在 `Broadcast` 前回收该对象，形成悬空指针。正确做法是捕获 `TWeakObjectPtr`，Broadcast 前先 `IsValid()` 检查。

## 提示

- 先把三个接口跑通，再考虑进阶的 payload 和 Remove-during-Broadcast 防护。implement-own 的价值不在于复刻全功能，而在于亲手走一遍类型擦除 + 句柄管理的设计路径。
- `TFunction` 的 Small Buffer 大小默认 32 字节（可查 `TFunction_OwnedObject` 实现）；超过 32 字节的 functor 会堆分配，lambda 捕获大量数据时记住这个边界。
- 做完 mini 实现后，在 IDE 中对 `DECLARE_MULTICAST_DELEGATE_OneParam(FMyRealDelegate, int32)` 展开（MSVC 用 `/P`），与你的 mini 版本对照。

## 复盘问题

1. 真正开始执行的时刻？（`Broadcast(Value)` 调用时，在当前线程同步地遍历所有绑定并逐一调用；没有延迟、没有任务队列投递，这是 C++ delegate 的关键特征——"调用即执行"。）
2. 谁负责这对象的生命周期？（`FMiniIntDelegate` 实例由其宿主对象管理；每个 `FMiniBinding` 的 `TFunction` 捕获的 lambda 闭包，生命周期由引用计数或用户保证决定——这是 C++ delegate 最大的生命周期陷阱来源。）
3. 涉及哪些命名线程？（本题全在 GameThread 或调用 `Broadcast` 的任意线程；`DECLARE_TS_MULTICAST_DELEGATE*` 才是多线程安全版本。）
4. 这对 GC 如何可见？（本题不涉及 UObject；若 lambda 捕获了 `UObject*`，GC 不可见该引用，对象可能在 `Broadcast` 前被回收。）
5. [本题专属] `IDelegateInstance`（UE 内部类型擦除接口）与你的 `TFunction<void(int32)>` 相比，多了哪些虚方法？这些额外方法的存在是为了解决哪些工程问题？

## 对应官方参考

- `Engine/Source/Runtime/Core/Public/Delegates/Delegate.h`（C++ delegate 系统完整文档注释，payload 机制，dynamic delegate 区别）
- `Engine/Source/Runtime/Core/Public/Delegates/DelegateCombinations.h`（宏的声明形状，`DECLARE_MULTICAST_DELEGATE_OneParam` 形参结构）
- `Engine/Source/Runtime/Core/Public/Templates/Function.h`（`TFunction<>` 实现，Small Buffer Optimization）
