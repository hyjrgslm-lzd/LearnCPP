# 14 从调用表达式追到运行状态

源码导读以固定版本、具体问题、拥有关系和退出路径组织。S1在已经会组合sender后看使用层；S2在G1/G3、run_loop和task机制之后看实现层。读不懂实现时回到对应主讲章节，不把源码阅读反过来当成那些章节的先修。

固定输入为stdexec `nvhpc-26.05`，SHA `6d7ad689f4d4831c5136e4abe1c601f9a3b64e43`。以下路径都相对于这个checkout，读者设置STDEXEC_ROOT后即可定位；[固定源码树](https://github.com/NVIDIA/stdexec/tree/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43)可在线对照。滚动main不会自动成为本课的证据版本。

## S1：先读表达式与生命周期

从`examples/hello_world.cpp`进入。先圈出执行资源、scheduler、sender描述和消费点，再画箭头，不先钻进每个模板。`just`保存值，`then`增加变换，`sync_wait(just(value) | then(f))`才连接和启动这一条图。scope的`spawn`会启动受管理工作；`on_empty()`或标准scope的`join()`本身返回等待描述，要被消费才执行等待，不能把所有名字带wait/join的调用都理解成马上阻塞。

S1观察程序先建立一个带调用计数的then。构图后计数为0，sync_wait返回后为1；这直接验证该例的惰性。下一步运行两种真实协程任务：固定实现的`stdexec::task`与扩展`exec::task`。两者都算出42，但相同结果不说明它们的默认环境、取消和调度亲和性完全相同。`examples/hello_coro.cpp`与两个task实现的区别由第10章逐项解释，S1先记录“协程何时创建、由谁启动、结果在哪里消费”。

最后看`examples/scope.cpp`和`include/exec/async_scope.hpp`。S1实际向两个worker的pool投递三个工作，经async_scope收束，并检查真实完成计数为3。计数在被调度的闭包里递增，不预填为3。数组/计数对象先创建，pool和scope后创建；退出时先收束scope、再让pool结束，借用对象最后析构。提交中途抛异常也先等待已经接受的工作，不能直接展开一个仍被任务借用的栈帧。

`examples/retry.cpp`只在此识别“失败后重新建立一次操作”的使用意图。完整的同步重入、异步pending、停止与操作状态回收要到G3后再读`examples/algorithms/retry.hpp`；本课不使用普通optional重试循环冒充sender源码实验。

### S1练习与解析

1. 标出四类对象：pool是资源，scheduler是调度能力，sender是描述，op-state是一次执行实例。一个可复制scheduler不意味着复制了一个线程池。
2. 把同一计算写成sender图和协程。两种写法都要指出消费点；创建task对象通常不代表任务已经完成。
3. 在scope中保留一个尚未完成的工作，解释为什么on_empty不能提前给出完成。实际受控例见C2_10和R1；不要靠sleep恰好观察到等待来推导所有交错。
4. 解释异常路径：投递失败只意味着当前投递失败，不会自动撤销此前已接受的工作。父作用域必须承担它们的收束责任。

## S2：沿五条实现链阅读

### just：值存在哪里

打开`include/stdexec/__detail/__just.hpp`，从`just_t`及其实现钩子定位存储。区分描述中的值和连接后运行状态中的值：重复连接、移动连接和捕获引用会带来不同的类型与存活要求。观察程序把1变成2只是入口；完成阅读还要写出“哪个对象持有1，哪一步移动它，哪一步调用下游”。

不要把临时表达式的简短外观当成安全保证。值按拥有方式保存时，临时输入可以安全转交；捕获外部对象的引用仍由调用者维持存活。这个判断需要回到C02所有权和本课operation-state协议。

### then：从签名推导到完成拦截

再读`__then.hpp`，对照G1的包装receiver。先找上游签名如何进入变换，检查void结果、可抛调用和转发环境；再找真正执行函数对象并向下游发信号的位置。生产实现用sender expression/domain等机制统一这些操作，G1则展开成容易定位的对象。

两者的契约差异必须写出来：G1只支持声明的连接/值类别组合，并保守增加exception_ptr错误通道；不能把教学代码的简化当成标准要求。完成信号可以导致运行状态被销毁，所以源代码审查应追问每一个terminal call之后是否仍访问该状态。

### when_all：计数归零与结果位置

打开`__when_all.hpp`，定位`__state`中的`__count_`、`__state_`、`__stop_source_`和`__arrive()`。计数归零触发最终收束；某一子项error或stopped会影响终态并请求其他工作停止，但请求停止仍不等于其他子项已经完成。

还要找各child结果写入哪个槽位。返回tuple的组织来自child在表达式中的位置，不能直接按回调到达顺序追加结果。S2检查两个child的结果组合，完整的乱序完成/取消交错应结合运行时和G3的受控完成源再实验。程序输出与源码规则分别登记，不以一次执行顺序推断全部调度。

### sync_wait：阻塞之外还提供什么

打开`__sync_wait.hpp`，从`__state`、`__receiver`和`__loop_`读起。接收value/error/stopped后，最终receiver更新结果并让内部run_loop结束；退出等待后才决定返回tuple、抛异常或返回空optional。其receiver环境还提供调度相关查询，所以sync_wait不只是一个条件变量包装。

这里尤其要区分两件事：C++异常是阻塞消费者把error通道映射回同步调用的方式；图内部error并不因此变成跨线程直接throw。研究无锁/取消优化前，要先画清谁写结果、谁等待、哪条同步关系保证结果可见。

### run_loop与task：退出路径决定是否能释放

打开`__run_loop.hpp`，看`__run_loop_base::run/finish`、队列的`__task`节点和`__opstate_t`。S2真的在一个外部拥有的jthread上pump run_loop，调度工作记录自己的thread id，验证它在pump线程执行。finish_guard先调用finish，随后jthread析构join，最后loop析构；异常展开也保持这个顺序。

最后分别读`include/stdexec/__detail/__task.hpp`与`include/exec/task.hpp`。从get_return_object、initial/final suspend、await转换和环境出发，追踪帧的所有者。查到final suspend并不足以宣布工作线程退出；H3的外部线程owner与真实join展示了这两个责任为什么要分开。

### S2练习与解析

- 对just/then各画一张“描述→连接→运行→完成→销毁”的对象图。答案必须包含值/函数对象和receiver的拥有位置。
- 对when_all列出value、error、stopped三条终态选择，并解释为何仍需等待已开始的child收束。只写“有计数器”不够。
- 对sync_wait解释空optional与抛异常的不同来源，再指出它提供的环境如何影响子操作。
- 对run_loop找入队、出队、调用回调和finish退出的位置；解释为何用户回调不应在队列锁内执行。
- 对task列出帧、awaiter、inner op、完成状态、外部执行线程的责任。以terminal回调立即销毁op为检查条件，阅读每条返回路径。

S1/S2都是观察与源码阅读单元。它们的可运行程序验证教材里的具体观察，不证明上游库整体正确，也不代替读者完成对象图和解析。主讲链接：[adaptor](08-adaptor.md)、[运行时](09-runtime.md)、[task/scope](10-task-and-scope.md)、[mini execution](15-mini-execution.md)。
