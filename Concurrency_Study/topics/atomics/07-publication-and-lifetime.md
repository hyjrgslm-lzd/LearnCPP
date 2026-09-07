# 发布与寿命：过去的写已可见，未来的写由谁阻止

本篇有两个完整程序：[F4 单槽确认](../../exercises/F4_publish_pattern/solution.cpp)与 [I1 不可变快照](../../exercises/I1_atomic_shared_ptr/solution.cpp)。前者保留同一对象、等待使用者确认后再修改；后者为每次发布分配新对象、让读者共享旧对象所有权。两者解决不同契约，不能作为不加说明的性能替换。

## 1. 一次发布的证明为什么在第二次修改时失效

假设生产者写 data 的第 v 版，然后 release store version=v；消费者 acquire 读到 v，开始复制 data。到这里，我们有：

```text
写第 v 版 ——HB——> 消费者读 data
```

如果生产者立即开始写第 v+1 版，现有关系并未规定“消费者读 data”与“写第 v+1 版”谁在前。两者都发生在第 v 版发布之后，处在关系图的两个分支上，不能因为有共同前驱就彼此有序。普通字段因此仍可能竞争；string 赋值甚至可能更换内部存储，读者持有的内部地址也会失效。

把版本号升级成 seq_cst 不会创建缺少的读完确认。读前后各检查一次版本号也不自动合法：若中间普通读取已经与写者冲突，事后发现版本变化并重试，不能撤销 UB。可移植的 seqlock 类方案需要对具体字段访问与对象寿命另作设计，不能照搬“普通字段，读错了再丢弃”。

因此旧 F4 的“单写者多消费者追版本并直接覆盖普通结构体”不是安全快照算法。本篇不再执行该错误实现。用可控状态图指出缺失关系，比依赖某次机器恰好未崩溃更有解释力。

## 2. F4：增加从读者返回的确认边

新 F4 限制为 SPSC，容量 1，固定 2000 次交接。published 是生产者写的版本号，acknowledged 是消费者读完后的确认版本号；两者初值 0。只有生产者写 slot，只有消费者读取 slot。

生产者准备第 v 版前，先 acquire 观察 acknowledged==v-1；写完 slot，release 发布 published=v。消费者 acquire 观察 published>=v，复制整个 slot，并检查它确实对应 v，然后 release 确认 acknowledged=v。由于生产者必须等确认才继续，published 不可能在消费者处理当前版时跨过下一版；本协议不跳版。

现在可以把生命周期连成一条链：

```text
Wv 写槽位 -> release publish(v) -> acquire 接收(v) -> Rv 复制槽位
                                                         |
W(v+1) 写槽位 <- acquire ack(v) <- release ack(v) <---------+
```

上行箭头组合出 Wv HB Rv，解决初始化可见性；下行确认组合出 Rv HB W(v+1)，解决复用时读写冲突。两种方向都不可少。第一次写之前的确认 0 来自初始化；此时还没有前一个读者，启动规则提供起始条件。

| 契约项 | F4 的选择 |
|---|---|
| 角色与容量 | 一生产者、一消费者、一个普通 payload 槽位 |
| 成功语义 | 版本 1..2000 每个恰好交接一次，字段属于同版 |
| 顺序与生效点 | release 更新 published 发布一版；消费者复制后确认 |
| 等待 | 空时消费者等 published，满时生产者等 acknowledged |
| 元素限制 | 固定 int/array，复制和赋值不抛异常，无外部引用逃逸 |
| 关闭与销毁 | 固定次数全部确认，消费者结束，get 返回后销毁 |
| 进展 | 依赖另一端推进；不承诺 lock-free、公平或固定等待时长 |

Reference 在 consumer 中记录 valid，继续完成全部确认，循环结束后再 cs::check。若直接在中途抛出检查异常，生产者可能永远等不到确认。后台检查最终通过 future::get 回传。这个安排适合本题固定数据检查，不是通用异常安全通道：换成可能抛出的元素操作，必须增加失败/关闭状态及唤醒路径。

## 3. I1：让已经发布的对象保持不变

配置读取通常不要求逐版送达，只要求每次拿到一个自洽版本。于是可以切换契约：写者构造一个全新的配置，通过原子共享指针替换入口；读者 load 出一份 shared_ptr，读取这份对象。

Reference 使用：

```cpp
using snapshot = std::shared_ptr<const config>;
std::atomic<snapshot> cell;
```

实际对象由 `make_shared<const config>` 构造，从一开始就是 const。仅把一个仍有 mutable 别名的 shared_ptr<T> 转成 shared_ptr<const T>，并不能阻止别处通过旧别名修改 T。因此“不可变”必须是所有访问者遵守的对象契约；这里用 const 对象本身加强表达。

写者完成构造后 release store，读者 acquire load 取得相应对象的初始化内容。与此同时，原子共享指针的操作把取得指针及相关强引用计数增加作为原子操作的一部分，避免“刚读出裸地址，还没增加引用时，对象已经释放”的窗口。读者手里的副本维持旧对象寿命，即使入口已经替换。

I1 首先持有第 0 版，写者替换成第 10 版，检查旧 weak_ptr 尚未过期且 held 仍可读第 0 版。释放最后一份 held 后，再检查 weak_ptr 过期。weak_ptr 本身不延长对象寿命，控制块的存在也不等于对象还活着。这个确定性场景验证实际的保留与释放条件，而不以“不崩溃”充当验收。

## 4. 引用计数保护寿命，不保护任意读写

几个不同的 shared_ptr 副本可以安全管理同一控制块；同一个普通 shared_ptr 变量被一边赋值、一边复制，仍需同步，atomic<shared_ptr> 就用于原子访问那个发布位置。取得副本之后，对 T 的访问也不自动加锁。若 T 可变且写者继续修改同一实例，引用计数只能让错误访问的对象仍然活着，不能消除数据竞争。

associated use_count 的减少以及可能的析构、释放并不都属于原子更新步骤本身；不要把整个析构过程当成 CAS 的不可分割部分。复杂析构器可能很昂贵，最后一个引用在哪个线程销毁还会影响延迟。即使 is_lock_free 返回 true，也不能因此把内存分配、用户析构和重试循环统称为无锁。

裸指针的 acquire load 也可以提供初始化可见性，但不增加所有权。另一个线程随后 delete，就会使解引用悬垂；换更强的内存序没有补出寿命保护。这就是引用计数、锁或专门回收协议的职责边界。

## 5. 多写者：替换和累积更新的差别

若两个写者从版本 10 各自算出版本 11，再先后 store，即使每个快照不可变且发布完全安全，最终也只是 11，而非 12。这是业务上的丢更新，不是数据竞争。I1 先串行构造这个受控交错，明确 store 的最后写入获胜语义。

要保留两次增量，必须把“基于旧状态计算”和“提交条件”关联起来。I1 的 `increment(cell)` acquire load old，构造 version+1 和对应 payload 的新不可变对象，然后 acq_rel CAS。成功就完成一次增量；失败会用当前快照回填 old，使用 failure=acquire 取得其初始化内容，再构造新的 next。

成功路径的 release 发布新对象，失败路径的 acquire 使下一轮可安全解引用回填的 old，old 本身的 shared_ptr 所有权维持寿命。成功比较按 shared_ptr 特化的等价规则，包括存储指针及共享所有权，不只是数值版本相等。程序没有把已淘汰的旧快照重新塞回入口；若业务允许重装历史快照，仍须另行定义更新语义。

两个写者各更新 1000 次，最终 version=2000、payload=4000；两个读者各做 3000 次 load 并检查 `payload == version * 2`。读者不要求看见每版，也不要求看到最终版；最终版由主线程在全部任务结束后检查。输入范围固定，version 及 payload 不会溢出。

| 契约项 | I1 的选择 |
|---|---|
| 存储与角色 | 多读者、多写者，一个原子快照入口；每次更新新建 const 对象 |
| 顺序 | store 是原子替换；增量以成功 CAS 的顺序累计 |
| 读者 | 返回一份拥有所有权的自洽快照，允许跳版 |
| 失败与重试 | CAS 失败重新计算；分配失败抛异常，未提交候选不发布 |
| 容量与寿命 | 不固定缓存历史数；每个持有者可能延长一个旧版本的寿命 |
| 进展 | 不承诺无锁；引用计数、分配及竞争重试都有成本 |

## 6. 运行与答案

在 `Concurrency_Study/exercises` 中：

```powershell
cmake -S F4_publish_pattern -B build/f4 -G "Visual Studio 18 2026" -A x64
cmake --build build/f4 --config Release
ctest --test-dir build/f4 -C Release --output-on-failure
cmake -S I1_atomic_shared_ptr -B build/i1 -G "Visual Studio 18 2026" -A x64
cmake --build build/i1 --config Release
ctest --test-dir build/i1 -C Release --output-on-failure
```

F4 必须完成 2000 个逐版确认；I1 检查寿命、受控 store 丢更新、并发自洽性与 2000 次 CAS 增量。所有异常回传主线程，不在正确性窗口里加 logger 同步。is_lock_free 为 false 是合法实现事实，不是失败，也不据此推断它一定比其他回收方案慢。

**把 F4 增加到多个消费者，只共用一个 ack 是否正确？** 不能直接推广。一个消费者确认不代表其他消费者读完。需要每人确认或另一个正确的读者集合协议；本题明确只支持一个消费者。

**快照可以返回裸指针给长期使用者吗？** 若同时把持有的 shared_ptr 销毁，就丢失寿命保证。返回值必须保留共享所有权，或由调用者提供其他有效保护。

**CAS 失败之后复用同一个 next 可以吗？** 对增量不行。它是根据旧版本算的，失败回填后必须重建。对纯粹覆盖固定配置的需求，可能根本不需要 CAS。

**快照和确认交接谁更快？** 本篇不能回答。两者交付契约、分配、等待和历史保留都不同；要测量必须先规定相同的业务目标并解释这些差异。

## 规范入口

[N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 的 [intro.races]、[util.smartptr.atomic.general]、[util.smartptr.atomic.shared] 给出共享指针的原子操作、计数与析构范围。[P0718R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0718r2.html) 是 atomic 智能指针特化的历史背景；具体内存序与等价规则以固定草案为准。
