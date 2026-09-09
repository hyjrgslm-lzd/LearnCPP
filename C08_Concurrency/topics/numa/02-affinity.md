# NUMA 02：把线程放到某处，还要确认它真的在那里

上一节找到了允许使用的逻辑 CPU。现在希望“初始化者和读取者都在指定节点”。最容易写出的代码是调用 affinity API，然后输出“绑核成功”。但 API 返回成功只回答请求是否被接收；实验的另一个问题是操作实际在哪个 CPU 上发生。因此本节把请求和观测分开记录。

真实代码是 [`current_cpu` 和 `on_cpus`](../../exercises/include/concurrency_study/numa.hpp)，J3 Reference 是最小驱动，N1 复用同一套逻辑包住初始化和读取。设置 affinity 的代码仅存在于 on_cpus 创建的专用新线程内；没有在调用者线程上临时设置再恢复的公共 guard。

## 1. 亲和性改变允许集合，不预订一颗独占核心

硬 affinity 给调度器一个可运行集合。只保留一个逻辑 CPU，可以限制该线程在普通调度中的迁移范围，但其他进程、内核线程、SMT 兄弟仍可能争用这个核心的资源。亲和性不保证实时响应，也不消除抢占。即使两次读到的 CPU ID 相同，中间也可能发生切换到其他任务。

Windows 使用 `SetThreadGroupAffinity`，把 group 和该组内的一位掩码一起指定。Linux 使用 `sched_setaffinity(0,...)`。这里调用 API 的是 on_cpus 新建的线程；它绑定一次，执行一次有限操作，然后退出。正常结束和异常路径都先 join 这些线程，调用者自身没有执行 affinity setter。J3 Reference 还检查操作线程 ID 与调用者不同，并检查异常从专用线程回传。

为什么不用一份旧 `GROUP_AFFINITY` 做通用恢复？Windows 11 和 Server 2022 可以让默认线程 affinity 覆盖多个 processor group，而 `SetThreadGroupAffinity` 会把线程限制到一个组。将返回的旧单组结构再设置回去，不足以还原原来的跨组状态。这是 API 表达能力的问题，不能凭 `GetThreadGroupAffinity` 成功就推断“原来已限制单组”。[Microsoft 官方说明](https://learn.microsoft.com/en-us/windows/win32/api/processtopologyapi/nf-processtopologyapi-setthreadgroupaffinity) 明确区分这两种状态。

因此本实现把副作用限制在即将退出的专用线程，不向复用 worker 提供“完整恢复”承诺。如果以后要在长期存活的线程池 worker 内临时改绑，应单独设计并验证原始 affinity 的完整表示与恢复方式，或者在能证明原来已被显式限制单组时才提供该接口。本次没有多组机器实测。建立初始绑定失败仍通过 unavailable 返回 77；业务异常由结果通道带回。

## 2. 请求与两次实际观测

`on_cpus` 为每个目标创建一个 packaged_task，然后交给 jthread 执行。worker 建立 affinity 后马上查询实际 CPU，操作结束后再次查询；任何一处与目标不一致都返回无法验证。Windows 查询使用 `GetCurrentProcessorNumberEx` 并由 NUMA API 查 node；Linux 使用 `getcpu`。

N1 额外将初始化和读取各自的 before/after CPU 保存进独立槽位，由主线程 join 后输出。不能从“初始化线程绑在 CPU A”推断“读取线程也在 CPU A”，更不能从 CPU node 推断页 node。四组 CPU 样本是执行位置证据，页面节点还要下一节查询。

两次观测是端点样本，不是持续迁移追踪。配合成功设置的单 CPU 硬 affinity，它们足以构成本实验的明确条件；若要证明长时间任务的调度轨迹，应增加 ETW/perf 等调度跟踪并保存采样范围。课程没有收集这种 trace，因此不会写“全程没有抢占/迁移”。

## 3. 为什么 worker 异常必须回主线程

一个线程中的异常不能跨线程调用栈直接传播。如果线程函数把异常抛到顶层，会调用 terminate。于是“有一个 CPU 不允许绑”可能让整个实验在输出原因前崩溃。

`on_cpus` 将操作封进 packaged_task，worker 的失败进入 future；主线程 get 时重新抛出，最外层区分不可验证与真正检查失败。jthread 容器在正常返回和异常展开时都会 join，其声明位置保证结果、捕获引用和操作对象仍然存活。部分线程创建失败时，已创建线程不等待一个永远凑不齐的 barrier：它们执行有限工作后退出，容器随展开 join。

这里不需要用 sleep “让线程先绑好”。绑定和检查发生在同一 worker 中，并按程序顺序发生在读取之前。线程启动时间的差异不是错误，且会被 NUMA 基准计时边界诚实包含。

## 4. 实验步骤和解释

先按 [J3 README](../../exercises/J3_numa_concept/README.md) 编译。main 选择允许集合的第一个 CPU；Reference 至多检查四个。预测输出中的请求和实际 group/cpu，然后运行 `ctest -V` 查看完整样本。

第二轮只改变外部允许集合，例如在一个已经限制好 affinity 的终端重新运行。目标应来自新集合，不能继续假设 CPU 0 可用。不要一边修改亲和、一边改变数据大小或线程数，再把时间差全归因于 NUMA。

有多个 node 时，后续 local/remote 基准固定 reader CPU，只改变页面首选节点；它不同时改变 CPU 与页面，否则“更慢”的原因将无法区分是换了核心、共享了 SMT，还是变成了远端内存。

## 自测与答案

**绑在一个 node 的 CPU 上，malloc 的数据就都在那个 node 吗？** 不成立。分配器可能复用已经驻留的页，或者内存策略指定了别处；页的第一次实际物理分配也可能由不同线程触发。要查询页面。

**两个 worker 的 CPU ID 不同，是否拥有独立 core？** 不一定，要查 SMT 同核关系。不同逻辑 CPU 可能共享执行资源和部分缓存。

**只在 main 查一次当前 CPU，能代表 worker 吗？** 不能，查询回答的是调用线程当时的位置。观测必须放在实际操作线程里。

**所有 worker 都建立好 affinity 后才开始，是否必须用 barrier？** 本节不需要共同起跑。若研究同步启动的吞吐，需要额外设计门闩及线程创建失败时的放行协议，同时把这一改变记录为新的计时边界。

**拿到旧 GROUP_AFFINITY 后再设置回去，是否恢复了 Win11 默认跨组 affinity？** 不能这样推断。旧结构只表示一个组，本例通过专用线程退出收束副作用，而不尝试这种不完整恢复。

## 官方依据

- [SetThreadGroupAffinity](https://learn.microsoft.com/en-us/windows/win32/api/processtopologyapi/nf-processtopologyapi-setthreadgroupaffinity)、[GetCurrentProcessorNumberEx](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getcurrentprocessornumberex)。
- [Linux man-pages：sched_setaffinity](https://man7.org/linux/man-pages/man2/sched_setaffinity.2.html) 和 [getcpu](https://man7.org/linux/man-pages/man2/getcpu.2.html)，分别定义请求接口与调用点观测。
- [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 的线程和 future 条款支撑异常/生命周期协议；CPU affinity 是平台扩展，不是 C++23 标准库保证。
