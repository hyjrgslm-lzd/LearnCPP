# 16：把类型、错误与事务组合成文档模型

这次把前面建立的规则组成一个可用的小型库：内存中存放Rectangle/Circle，允许增、改、删，调用者能复制文档、查询快照，并把多条修改作为一次事务。它是数据模型，不是绘图框架。先修为01—07的类型/值/状态/错误/异常与接口；08—15提供方案比较能力，但实现本项目不必硬塞入每一种多态机制。

## 1. 从约束反推类型

第一版若公开`vector<RawElement>`，任何调用者都能写出重复ID、零尺寸或绕过验证的修改。只在某个按钮回调检查一次不够：复制、批处理与未来另一客户端都可能走其他路径。

把约束分两层。单值约束交给强类型：ElementId只接受非零uint64_t，PositiveLength只接受正int，工厂返回expected。集合约束留在Document：一个Element本身不知道其他元素ID，因此唯一性由拥有集合的对象检查。label的缺失属于有效状态，使用optional<string>，不拿空字符串同时表示“未命名”与“名字恰好为空”。

```cpp
auto id = ElementId::make(raw_id);
auto width = PositiveLength::make(raw_width);
if (!id || !width) { /* classify the ValueError */ }
```

工厂失败时没有半初始化的有效强类型逃逸。代码不暴露可写内部vector，不让调用者在成功构造后再次破坏集合不变量。工厂接收的整数已经是本层结构化输入；文本、编码和跨类型数值转换由C05深入，这里不把类型合法等同外部输入已经可信。

## 2. 为什么最终选择variant

本场景明确只有Rectangle和Circle，增加备选会修改这套模型及其访问者。`variant<Rectangle,Circle>`让这个封闭集合进入公开契约；每个Element同时保存ID、Shape和可选label。两个文档是否相等由顺序、ID、图形备选及字段决定，地址、分配器内部状态不参与值比较。

若场景改成第三方能独立增加图形，封闭集合契约就改变了。09的虚接口、11的any_shape、13的polymorphic各提供不同的开放扩展和复制责任，不能说换成它们就自然“升级”或更快。此项目选择variant，是为了忠实表达当前输入域。也不能把`std::regular<Document>`编译通过当作值公理证明；checker还会实际修改复制品，检查原对象不变。

## 3. 查询与命令采用不同状态模型

查询不存在的ID很常见，`find`返回optional<Element>值快照。访问者拥有返回值，之后文档修改不使快照悬垂。代价是复制，字符串label等对象可能分配；因此find/snapshot不是noexcept。返回const引用虽然减少复制，却要求调用者理解文档更新、移动和析构后的失效规则，第07章已解释这类取舍。

命令失败需要原因，采用expected<void,EditError>。Add重复、Replace/Erase缺失各返回错误码和ID；它们不属于契约违约或程序崩溃。相同接口仍允许bad_alloc等异常传播：expected负责描述业务拒绝，不是一个自动捕获所有异常的容器。

```cpp
using Edit = std::variant<Add, Replace, Erase>;
std::expected<void, EditError> apply(const Edit&);
std::expected<void, EditError> apply_batch(std::span<const Edit>);
```

`const Edit&`和span只在调用期间借用命令，Document复制需要留下的Element，不存命令引用。apply把单条命令视为长度1的批，使成功、错误与失败保证走同一条公共路径。空批立即成功，不需要复制整个文档。

## 4. 先证实提前提交的问题

正确的basic版本可以逐条修改live集合，遇错返回；它的契约允许已经成功的效果保留。但是本项目明确要求强事务保证。使用相同输入：原文档含ID1和2，批中先替换1、增加3、再删除不存在的99。若直接循环修改live，最后虽正确返回missing_id，原文档却已经改变。这是错误通道正确、状态保证错误的具体区别。

练习保留一个只违反批事务保证的bad实现。checker不仅检查返回错误，还比较全部原始值，因此不能用“已经返回错误”掩盖部分提交。这个错误输入先于改进版本的验收存在，执行入口见下文负例命令。

## 5. 从提交点推导strong实现

把所有可能失败的修改放到候选文档中；候选的前一条修改对下一条可见。任何业务拒绝或异常都在提交前退出，候选通过RAII销毁，原文档未变。全部成功时只交换vector存储。

```cpp
if (edits.empty()) return {};
Document next(*this);
for (const Edit& edit : edits) {
    if (auto result = next.apply_in_place(edit); !result) return result;
}
swap(next);
return {};
```

这里的`swap`使用相同的标准allocator，不涉及不相等、不可传播allocator的前提问题。原存储在swap后由next拥有，函数结束析构。候选内部的Element赋值可以抛异常；只要它仍能安全析构，就不需要让每一个中间操作单独满足整个文档的strong保证。

Reference通过visit识别命令。泛型lambda从`decltype(operation)`移除cv/ref后用if constexpr区分Erase的id与Add/Replace的element.id；被丢弃分支不需要对当前类型有效。这是第10章的局部模板机制，不是运行时同时访问两种字段。独立good用显式get_if选择备选，维护候选vector，展示同一契约允许不同代码组织。

## 6. 复制与移动也属于接口

copy constructor复制全部Element；copy assignment先准备独立副本再swap，目标原值直到成功才替换。移动转交存储并把源清空，使源仍可接受下一条Add。自移动单独识别，避免先清空自己。相等按有序值比较，所以复制后的文档相等，但它们不共享可修改状态。

这不承诺回滚调用者在调用前构造Edit时的副作用，也不承诺跨线程同时调用安全。`noexcept`移动表示异常逃出会终止，不代表它能修复任意底层实现的故障。本机Debug库的代理分配正提供了一个实际边界：不可把全局OOM注入直接放进所有noexcept内部路径后仍要求恢复。

## 7. 失败实验先测路径，再注入

先运行正确批事务，测得此输入实际发生的普通分配次数；再重建原始文档，逐分配点抛bad_alloc。每一轮比较原值、再次成功执行同一批并检查最终结果；复制赋值的失败另作检查。测量次数只安排该实现当前路径，不是文档接口承诺，也不支持时间性能排名。

初版全局注入触发了MSVC Debug string的noexcept代理分配，导致真实abort。课程保留[最小复现与根因](../references/validation/p1-debug-allocation/diagnosis.md)：正常Debug现在维持完整迭代器检查，普通分配注入使用独立IDL0实验目标。同源码分两种明确环境验证，不把关闭诊断或SKIP假装成修复。

从课程exercises目录复现：

```powershell
cmake -S P1_document -B build/document -G "Visual Studio 18 2026" -A x64
cmake --build build/document --config Debug
ctest --test-dir build/document -C Debug -V
ctest --test-dir build/document -C Debug -R validation_bad_rejected -V
```

预期：Reference和独立good通过，bad在`failed batch preserves the entire original document`被拒；两个allocation目标执行实际失败注入。检查函数使用Release仍有效的check，不靠assert。每个Part的实现边界、Student起点、完整解析与版本位置见[P1练习](../exercises/P1_document/README.md)。

## 自测与解析

1. **已经返回missing_id，为何仍可能错误？** 返回类型只描述错误通道。强保证还要求批失败时原文档不变，必须检查整段状态，不能只看result。
2. **为何不用optional<void>？** optional表达有/无值而不携带失败原因，且C++23 optional不支持void；expected<void,E>正好表达无返回值的命令结果。
3. **为何不把所有异常转换成EditError？** 无法恢复的分配异常和业务拒绝不是同一类；捕获后若丢失上下文，调用者可能错误重试或报告缺失ID。只在有明确映射契约的边界转换。
4. **为何不保存回调到Document？** move-only回调会改变copyable值模型；借用回调会引入外部寿命，提交后回调抛异常又要求解释状态是否已提交。本课把这些作为12章的独立接口问题，不悄悄扩大当前事务责任。
