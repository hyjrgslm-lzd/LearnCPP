# 17 从教学模型进入真实源码

本章追两条路径：一次 io_uring 请求怎样从用户态准备进入完成队列；一次 pmr 分配怎样从类型化容器进入字节资源，又在什么边界归还。目标不是遍历整个项目，而是沿明确的问题找到入口、状态、分支与退出，并把观察接回前面的实验。

先完成 [L05](../exercises/L05_pmr/README.md) 和 [L08](../exercises/L08_completion/README.md)。读队列发布的并发细节时，按需回访 C08 的 release/acquire；不能把 Linux 用户态/内核共享队列的实现条件直接套成一个普通 C++ 多线程队列。

## 固定这次到底读了什么

liburing 固定为 `liburing-2.15`，commit `d41bf9220ec39277ff235379e9089d9e0fd6c2a5`。本机标准库则读实际编译器使用的文件，不能用滚动 master 代替本地构建。复查时记录本机实际源码路径、工具链版本和输入文件指纹；这些记录属于本机验证产物，不随课程正文提交。

| 输入 | 本次入口 |
|---|---|
| liburing | guest `/root/learncpp-c07/deps/liburing-2.15/src/queue.c`、`src/setup.c`、`src/include/liburing.h` |
| MSVC STL | Tools `14.51.36231/include/xpolymorphic_allocator.h`、`include/memory_resource` |
| libstdc++ | `/usr/include/c++/13/bits/memory_resource.h`、`memory_resource`、`bits/alloc_traits.h` |

版本改变后，下面的行号只作旧版本导航，应按符号重新定位并保存新指纹。源码没有出现在仓库里，不意味着读者必须猜：固定依赖由 [构建指南](../exercises/BUILD_GUIDE.md) 准备；标准库来自记录的工具链。本文给出必要的控制流解释，外部源码用于验证与深入。

## 路线一：SQE 被填好，还欠谁什么

从 [本课 completion 驱动](../exercises/include/c07/completion_io.hpp) 的 `io_uring_get_sqe`、`io_uring_prep_read` 和 `io_uring_submit` 进入。SQE 是请求描述，里面包含 fd、缓冲地址、长度、offset 和 identity。**准备描述与内核接受不是同一个事件**，而原缓冲区的存活又不同于 SQE 槽的复用。

在 guest 中定位：

```bash
grep -nE '__io_uring_flush_sq|__io_uring_get_cqe|io_uring_submit\(' \
  /root/learncpp-c07/deps/liburing-2.15/src/queue.c
grep -nE 'io_uring_queue_init_params|io_uring_queue_exit' \
  /root/learncpp-c07/deps/liburing-2.15/src/setup.c
```

本次 `queue.c:247` 的 `__io_uring_flush_sq` 负责发布本地准备的队列进度；`io_uring_submit` 在本次 `queue.c:511` 进入提交路径。先区分本地 SQE head/tail 与共享的 kernel head/tail，再检查所采用的 flags。默认非 SQPOLL 路径和 SQPOLL 路径的发布方式不同，不能挑一条赋值语句就宣称“普通非原子读写可以做任意并发队列”。库依赖具体 Linux 协议、系统调用和平台屏障条件。

接着沿 `__io_uring_get_cqe` 查看等待和取得完成项的路径。调用者要先复制 CQE 的 `user_data` 与结果，再告知库这一项已经消费；消费后不能继续把那个 CQE 槽当作自己的稳定对象。用户 buffer 的释放条件是目标请求已完成并被应用正确收束，不是“我已经写了 SQE”，也不是“收到另一条取消请求的 CQE”。

本课把这些责任拆成三个对象：

```text
operation_context：稳定身份 + 用户 buffer
SQE：提交给内核的操作描述
CQE：内核给出的某个请求结果
```

因此 [P1](../exercises/P1_file_pipeline/README.md) 即便按 offset 组装数据，也必须先按 identity 找对请求对象；offset 正确不能补救缓冲已经提前销毁的问题。其多请求失败控制在第一块物化时注入 `bad_alloc`，检查余下目标完成先收束，再让上下文析构；它证明这条受控异常路径，不证明所有设备和所有交错都已穷尽。

### 退出路径为什么还要单独看

本次 `setup.c:467` 的 `io_uring_queue_exit` 做 SQE/ring 映射和 ring fd 的回收。阅读这个函数时应问：它有没有逐个执行我应用自己的结果处理、借用解除和对象析构协议？答案是否定的。应用不能把调用这个库清理函数等同于自己的 accepted set 已归零。

L08 对取消控制和原目标分别记录结果；P1 的错误 guard 保持 slot/buffer 存活，先取消并消费剩余完成。若不能确认收束，受监督实验进程以明确的非成功状态结束，避免展开仍可能被内核借用的对象。这个 fatal 边界不是面向服务运行时的通用恢复方案；C09/C10/C11 的运行时需要在自己的契约下继续设计。

## 路线二：容器、allocator、resource 不是同一层

从 `std::pmr::vector<std::pmr::string>` 的一次扩容开始追踪：容器决定元素布局与容量，allocator 带着类型进入构造/分配协议，memory_resource 接收字节数和对齐。资源层没有足够的类型信息去执行任意用户类型的构造函数。

MSVC 本次 `xpolymorphic_allocator.h:137` 把分配请求交给 `do_allocate`，随后有一次非分配形式的 placement `operator new` 调用。不要把它读成又向操作系统申请一块内存，也不要当作已经执行了 `T` 的构造。回到 `allocator_traits` / `construct_at` 的类型化调用点，才能解释 L05 中构造抛异常时，为什么应只归还存储而不析构一个从未构造成功的对象。

再看 `select_on_container_copy_construction`。本次 MSVC 入口在 `xpolymorphic_allocator.h:311`；libstdc++ 对应入口在其 `bits/memory_resource.h`。复制容器、显式给 allocator 的复制、移动与赋值有各自规则，不能把“元素类型都是 pmr string”推成任何拷贝都沿原资源分配。L05 的 checker 自己提供上游观测器和嵌套容器，直接查看实际 resource 和地址/分配平衡，避免只相信函数返回的布尔报告。

### 为什么单调资源的 deallocate 几乎没代码

MSVC 的 `memory_resource:667` 与本次 libstdc++ `memory_resource:435` 都有单调资源单块归还的空操作。这是该资源的批次责任：单个容器停止使用一个块，不表示资源已经把它归还上游。读取 `release()` 和析构路径，才能找到大块归还的位置。

libstdc++ 的 `do_allocate` 先尝试在剩余区域满足对齐与大小，放不下才取得新 buffer；MSVC 的对应实现也保存当前区间与可用空间，但增长参数、内部头和管理结构不同。增长策略与具体 chunk 大小属于实现细节。[B01](../exercises/B01_costs/README.md) 同时记录调用数、上游字节、setup、工作循环和最终清理，不能以“deallocate 是空操作”直接推出端到端更快。

## 一个真实的规范与实现差异

[N4950 的 mem.res.monotonic.buffer.mem](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/n4950.pdf) 和已纳入 C++23 的 [LWG3120](https://cplusplus.github.io/LWG/issue3120) 要求 `release()` 恢复初始 buffer 和后续增长状态。初始 buffer 不是由 upstream 分配的，所以只数 upstream 的 deallocate 调用不足以验证这个要求。

本次源码阅读发现：MSVC 在没有 upstream chunk 时的提前返回，与 libstdc++ 恢复初始 buffer 的路径不同。为此增加了[独立观察程序](../exercises/B01_costs/source_observation.cpp)：给 64 字节初始 buffer，使用 `null_memory_resource` 禁止偷偷从堆补空间，分配满后 release，再申请同样的 64 字节。

```powershell
# 从 exercises；先按构建指南配置 verify-core
cmake --build --preset verify-core --target B01_source_observation
ctest --preset verify-core -R B01_source_observation --output-on-failure
```

本机一次观察中，MSVC 与 libstdc++ 对这条 `release()` 边界的表现不同：Windows 侧未观察到同一初始 buffer 被恢复，Linux/libstdc++ 侧观察到恢复。复查时应重新运行观察程序并保存本机输出；观察程序 exit 0 仅表示已取得观察且新的 resource 作用域对照通过，不是把库差异改叫标准通过。

本课没有修改系统标准库。跨平台实际路径在每批重新构造标准单调资源，且先结束所有借用它的对象；两端的新作用域对照都通过。B01 不依赖“同一标准资源 release 后重新使用外部初始 buffer”的缺陷分支，自制有界 arena 的 reset 行为则由自己的 checker 验证。

## 阅读练习与完整解析

**Part 1：SQE、buffer、CQE 哪一个可以在提交之后立即复用？**

解析：不能把三者混成一个寿命。SQE 的准备/提交槽遵循所用 ring/library 协议；用户 buffer 仍需覆盖目标 I/O；CQE 槽在告知 consumed 后可能被复用。回答必须指出当前库版本和 flags，不能只说“异步就都复制了”。本课默认没有 SQPOLL、multishot 或注册 buffer 扩展。

**Part 2：只看到 cancel 的 CQE，能销毁原请求对象吗？**

解析：不能。取消是另一项操作，结果可能是目标已经结束或无法取消；原目标自己的最终状态仍须收束。对照 L08 两种合法顺序和缺目标完成的 bad；Windows 对应取消控制调用返回，没有凭空多一张取消 IOCP 包。

**Part 3：为什么 pmr vector 的一次复制可能换 resource？**

解析：容器调用 allocator 的选择与传播协议。是否显式指定 allocator、是复制构造还是赋值/移动会改变路径。沿实际调用进入 `select_on_container_copy_construction` 再看对应构造，最后用 L05 的上游观测器验证，不能仅凭同一元素类型推断。

**Part 4：allocator 调用变少而耗时没有变好，首先查什么？**

解析：先核对是否同一工作量、是否计入 backing 初始化与批次清理、测量精度与样本离散，再查看成本是否落在对象工作、复制或最终写入/flush。上游 allocator 调用不是 OS 系统调用计数；重复地址也不证明没有调用 allocator。按 B01 的原始阶段结果判断，不用源码看上去更短代替测量。

完成本章应能够提交一份短阅读记录：输入指纹、具体问题、入口/关键状态/退出、与课程模型的对应、一条真实运行证据，以及仍未证明的范围。只贴项目地址或复述函数名不算完成源码阅读。
