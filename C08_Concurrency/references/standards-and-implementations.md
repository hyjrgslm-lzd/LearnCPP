# 标准、实现与教学协议

> 2026-09-10修订：线程属性、HP batches及标准HP/RCU/sender新增正文和独立主体，见[前沿入口](../topics/frontier/README.md)及[本轮质量报告](revision-quality-report-20260910.md)。下面2026-09-08的工具探测是历史记录，不能替代本轮结果；规范仍分别固定N5050/N5054。

核对日期：2026-09-08。课程代码默认以 C++23 为基线；讨论 C++26 时固定引用 N5050。N5050 是 C++26 最终草案及 DIS 的基础，后续 N5054 已进入 C++29。滚动工作草案方便定位，但不能把它后来增加的功能全部归为 C++26。[N5051 编辑报告](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5051.html)、[N5055 编辑报告](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html)。

## 1. 四种不同的证据

读到一个技术结论，先判断它在回答哪种问题：

| 类别 | 能回答的问题 | 不能替代什么 |
|---|---|---|
| 规范 | 合法程序必须满足的语义、前提、允许结果 | 某个版本的库是否已实现 |
| 算法与教学协议 | 在明确假设下，不变量和操作如何成立 | 生产级泛化、所有类型与所有失败模式 |
| 实现资料与源码 | 某个编译器、标准库或第三方版本如何工作 | 其他实现必然采取同样方式 |
| 运行观察 | 本次输入、构建、硬件及调度下发生了什么 | 任意交错的正确性、普遍性能结论 |

课程会给出具体推导与实验。看到“不出现某结果”“某版本更快”时，检查它来自规范禁止、协议证明，还是有限次运行；它们是不同强度的结论。

## 2. 语言版本与实际能力

线程、mutex、future 和原子操作的主要基础来自 C++11；并行算法与硬件干扰尺寸常量来自 C++17；jthread、stop_token、atomic_ref、atomic wait/notify、latch、barrier、semaphore 来自 C++20。课程用 C++23 的 move_only_function 承载不可复制任务。

这些版本标签说明设施进入标准的时间。实际可用性还取决于编译器前端、标准库、编译选项及链接后端。Clang 的版本不能单独说明它搭配的是哪一版 libc++ 或 libstdc++；一个预览语言选项也不能补出缺失的标准库头。

本机 CMake 的 C++23 target 映射及明确的 C++23 预览验证方式见[构建指南](../exercises/BUILD_GUIDE.md)。

## 3. 容易混淆的规范边界

| 主题 | 课程采用的准确边界 | 规范或提案入口 |
|---|---|---|
| async | 区分发起、可调用对象执行、共享状态就绪和消费；不把“可以开始执行”写成“返回前一定执行到某行” | [futures.async](https://eel.is/c++draft/futures.async) |
| future 释放 | 检查来源、就绪状态和是否最后一个引用；不将所有 future 析构写成等待任务 | [futures.state](https://eel.is/c++draft/futures.state) |
| release sequence | C++20 起，release 后延续序列的操作须为相应连续 RMW；同线程普通 store 不自动延续 | [P0982R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0982r1.html) |
| consume | C++26 弃用且规定相应 acquire 语义；它不是被删除，也不是枚举值与 acquire 必然相等 | [P3475R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3475r2.pdf) |
| 内存序 | acquire 与 release 约束不同方向；合法序还依赖 load/store/RMW/wait 等具体操作 | [atomics.order](https://eel.is/c++draft/atomics.order) |
| atomic wait | 内部可伪唤醒，对外返回前会重新比较；短暂 A→B→A 可以漏观察，notify 不独立发布 payload | [atomics.wait](https://eel.is/c++draft/atomics.wait) |
| barrier | completion 的执行者与无人 wait 时的行为有明确边界；不能要求永远是最后到达者执行 | [P2588R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2588r3.html) |
| atomic_ref | 底层对象、对齐及访问阶段必须合法；N5050 已包含 const 目标的相关支持和 address()，本课程基础实验使用较早接口 | [atomics.ref.generic.general](https://eel.is/c++draft/atomics.ref.generic.general) |
| atomic min/max | C++26 的原生 RMW 与可以提前只读返回的手写 shortcut 不是同一契约 | [P0493R5](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p0493r5.pdf) |
| policy 算法 | par 允许而不保证并行或加速；标准 policy 重载的异常规则与无 policy 重载不同 | [algorithms.parallel.exec](https://eel.is/c++draft/algorithms.parallel.exec) |

表中的滚动定位链接须结合固定规范和相应版本说明阅读。教学正文给出代码、不变量及详细例子，不能只记这张表的短句。

## 4. C++26 与回退实现

### SIMD

课程讨论的 C++26 接口使用头 `<simd>`、命名空间 std::simd，以及 vec/basic_vec 等类型。加载、存储、select 等按相应版本的自由函数接口说明。旧 TS 的 std::experimental::simd 和早期提案的 copy_from/where 等接口不能直接拼进这一版本。[P3691R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3691r1.pdf)、[SIMD synopsis](https://eel.is/c++draft/simd.syn)。

xsimd 是独立库，不是标准头的别名。课程固定 xsimd 13.2.0，并将其接口、实际 ISA 和精度约定与标量/标准版本分别核对。完整演进见[SIMD 入口](../chapters/15-simd.md)。

### 执行模型

C++17 的 execution policy 算法与 C++26 sender/receiver 属于不同的接口层。标准 sender 等待入口为 std::this_thread::sync_wait；stdexec 的对应入口使用自己的命名空间。exec::static_thread_pool 等具体执行资源还须按第三方扩展理解。[P2300R10](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html)、[标准 sync_wait](https://eel.is/c++draft/exec.sync.wait)。

标准后续对 bulk 等算法也有演进，源码导读需要绑定版本，不能只用最初提案解释固定依赖中的全部接口。[P3481R5](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3481r5.html)。

### 安全回收

N5050 保留 hazard pointers 与 RCU。课程的 cs 实现用于展示可检查的协议与生命周期，包含额外的清理/观察接口和明确的教学限制，不能只替换命名空间就声称标准兼容。[N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)、[回收专题](../topics/reclamation/README.md)。

EBR、QSBR 与 RCU 的关联由专题分别说明。版本标签用于识别某些状态变化，不能单独保证节点仍存活。将 C 库或内核算法转写为标准 C++ 时，必须重新核对对象生命期和数据竞争规则。[Concurrency Kit](https://github.com/concurrencykit/ck)、[Userspace RCU](https://liburcu.org/)。

## 5. 本机构建探测

### C++29 增量的独立状态

2026-09-08 核对 [N5055 编辑报告](https://www9.open-std.org/JTC1/SC22/WG21/docs/papers/2026/n5055.html)：LWG Poll 9 和 Poll 18 分别将 P3428R4、P2019R9 应用到 C++ 工作草案；该报告对应 N5054（C++29）。这说明已入稿，不是最终发布标准或本机实现声明。

| 增量 | 规范状态 | 本课程实现与实验状态 |
|---|---|---|
| Thread attributes，P2019R9 | 已纳入 N5054 工作草案 | [F01](../exercises/F01_thread_attributes/README.md)新增名称/栈大小hint的正文、标准主体及专用probe。亲和/拓扑实验与教学模型不等于验证标准API；实际结果及审查见本轮报告。 |
| Hazard Pointer Batches，P3428R4 | 已纳入 N5054 工作草案 | [F02](../exercises/F02_hazard_pointer_batches/README.md)新增批量make/clear、资源状态及独立probe/主体。教学HP协议、早期HP探测不替代batch接口；实际结果及审查见本轮报告。 |

2026-09-08只登记了索引；本轮新增材料后仍分别记录作者完成、独立审查和实际工具能力，不因登记或模型通过而标为原生PASS。[C01](../../C01_Build_Compile_Link/README.md)解释怎样区分特性宏、头文件、真实实例化、链接和运行证据。

本轮以 `CONCURRENCY_STUDY_ENABLE_CXX26` 和 `CONCURRENCY_STUDY_ENABLE_CXX29` 分别请求两组原生设施。OFF是DISABLED，ON后最小能力缺失才是SKIP；能力满足之后主体的编译或行为错误必须FAIL。原生HP/RCU/sender主体见[F03](../exercises/F03_native_facilities/README.md)，固定stdexec仍是独立实现分支；已有inplace stop、min/max与SIMD主体继续复用。各设施单独记录，不能以其中一项通过掩盖其他项未执行。

### 既有 C++26 探测记录

探测源是 [feature_probes.cpp](../exercises/cmake/feature_probes.cpp)，每个选项独立编译并链接。OFF 表示没有请求该原生路径；ON 后失败才产生该接口的失败日志。

| 能力 | 本课程门槛 | 2026-09-08 本机结果 |
|---|---|---|
| policy 算法 | 实例化并链接 sort/reduce 的 par 重载 | 通过；不表示任意运行一定多线程 |
| 标准 SIMD | __cpp_lib_simd >= 202603L，实例化 vec/select/reduce | 未通过，`<simd>` 缺失 |
| 标准 senders | __cpp_lib_senders >= 202506L，just/then/sync_wait | 未通过门槛 |
| 标准 HP | __cpp_lib_hazard_pointer >= 202306L，工厂/保护/退休 | 未通过，头缺失 |
| 标准 RCU | __cpp_lib_rcu >= 202306L，域与同步接口 | 未通过，头缺失 |
| 原子 min/max | __cpp_lib_atomic_min_max >= 202506L，实际调用 | 未通过门槛 |
| inplace stop | 实例化 source/token/callback/never_stop_token | 未通过，类型缺失 |

精确失败原因保存在所用构建目录的 capabilities 下。早期或部分 SIMD 接口未达到完整门槛时，不宜把失败泛化成“整个编译器完全没有 SIMD”。编译失败也可能是选项或链接条件问题，须阅读日志。

本机结果不能外推为所有 MSVC、GCC 或 Clang 版本的状态。进一步核查使用 [Microsoft STL 符合性](https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance)、[libstdc++ 状态](https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html)、[libc++ C++26 状态](https://libcxx.llvm.org/Status/Cxx26.html)，并以实际实例化和链接复核。

第三方精确提交与本地来源检查见[构建指南](../exercises/BUILD_GUIDE.md)；实际测试和未验证范围见[质量报告](quality-report.md)。
