# C++29 Hazard Pointer Batches：一次管理一组保护槽

[P3428R4](https://wg21.link/P3428R4) 在标准 HP 上新增两个自由函数；[N5055](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html) 的 LWG Poll 9 说明它已应用到 N5054 工作草案。它扩展的是 C++26 HP 设施，不重新定义 HP 的保护/退休规则。

```cpp
void make_hazard_pointer_batch(std::span<hazard_pointer> hps);
void clear_hazard_pointer_batch(std::span<hazard_pointer> hps) noexcept;
```

它解决的是“一个算法阶段需要一组 HP 句柄”的资源管理问题，不改变 HP 的根本保护协议。每个句柄仍只能保护当前公布的一个对象；对象退休、扫描和最终删除仍服从原来的安全回收规则。

## 1. 为什么 batch 不是循环语法糖

多个保护槽常出现在链表、树、跳表和队列算法中。手写循环逐个 `make_hazard_pointer()` 的问题不是少一行代码，而是失败边界：给 empty 元素申请到一半抛异常时，调用前已有句柄必须保持一致状态，调用方不能得到半初始化数组并继续运行。

P3428R4 对 `make_hazard_pointer_batch` 明确要求强异常保证：如果无法完成整批申请，调用前 span 中的 HP 对象保持原状态。空 span 是合法无操作。重复调用需要按当前状态理解：非空元素已经拥有 HP，make 不应把它们释放或重建。

`clear_hazard_pointer_batch` 是批量销毁 span 中非空元素拥有的 HP，并让这些元素回到 empty 状态；它是 `noexcept`。它不同于 `hp.reset_protection()`：后者只解除当前 protected pointer，HP 对象仍然非空、仍拥有保护槽。clear 后不再有这个 owned HP，后续若要保护对象需要重新 make。

## 2. mixed span 的危险

“mixed span” 指一组句柄里有的空、有的已经拥有槽、有的正在保护对象。标准函数需要处理 span 中的对象状态，但调用方仍要理解自己在清空什么。清空一个仍要解引用的对象保护，会把 bug 提前暴露给回收器；标准不会替调用者延长裸指针生命期。

本课程的 F02 模型检查四件事：

1. 空 span 不改变状态。
2. 申请失败时已经存在的句柄状态保持不变。
3. 非空 batch 只为空元素构造 owned HP；已经非空的元素连同其 protection 保持可用。
4. clear 可以重复调用，销毁“一个正在保护对象、一个未关联对象”的 mixed span 中 owned HP，并让元素变 empty。

这些检查验证的是 batch 资源边界，不证明某个具体标准库的内部扫描算法。

## 3. 与 protect/retire 集成

batch 只批量准备 owned HP，或批量销毁 owned HP 并让元素 empty。算法仍必须按 HP 规则使用它：

```cpp
make_hazard_pointer_batch(hps);
auto* head = hps[0].protect(source);
auto* next = head ? hps[1].protect(head->next) : nullptr;
// 验证结构关系，再 CAS 摘除
clear_hazard_pointer_batch(hps); // hps[*].empty() == true
```

若结构关系在保护后失效，算法要重试；不能把 “我有两个 HP 句柄” 当成 “这两个指针形成同一时刻的稳定快照”。成功摘除后的节点仍只退休一次。最后清场仍需要停止生产、join 使用者、清空保护，再让回收域执行完成。

## 4. 不验证什么

本轮不注入标准库内部 OOM，也不假设标准库提供强制排空所有退休对象的接口。F02 的失败注入只在课程模型里验证强异常边界；真实标准库主体只在 `CS_HAS_STD_HAZARD_POINTER_BATCH=1` 时运行空 span、非空 batch、mixed associated/unassociated span、重复 clear 后 empty、保护和退休这条最小路径。

如果未来实现支持 HP 但不支持 batch，F03 原生 HP 主体可以运行，F02 标准 batch 主体仍应 SKIP。两者是不同能力。

## 5. 来源

- [N5055 编辑报告](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html)：P3428R4 已应用到 N5054 工作草案。
- [P3428R4](https://wg21.link/P3428R4)：新增 `make_hazard_pointer_batch(span<hazard_pointer>)` 和 `clear_hazard_pointer_batch(span<hazard_pointer>) noexcept`，规定 make 的强异常保证，并规定 clear 销毁 owned hazard pointer 后使元素 empty。
