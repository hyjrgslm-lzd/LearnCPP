# M1：从静态分工到带锁工作窃取

先读[静态/动态调度](../../topics/scheduling/01-static-dynamic.md)，再读[完整线程池协议](../../topics/scheduling/02-work-stealing.md)。实现位于 [work_stealing_pool.hpp](../include/concurrency_study/work_stealing_pool.hpp)；[main.cpp](main.cpp) 演示同一偏斜负载的三个调度版本，[solution.cpp](solution.cpp) 是完整 Reference，[runtime 检查](../runtime_tests/scheduling_test.cpp) 补充多提交者及关闭竞争。

本题默认 C++23，无额外库依赖。它是每 worker 一条带锁 deque 的真实窃取池，没有实现 Chase–Lev，也不保证全局 FIFO、无锁进展或任意依赖图无死锁。

入口类型：main 是独立 baseline 的 OBSERVATION 驱动，检查三个调度版本各 32 个任务的结果，没有直接 include solution.cpp。默认成功仅覆盖这些真实运行的结果检查，不表示 Part 2–5 的实现、协议推导或完整压力检查已完成。按各 Part 遮住对应函数重写并用独立 Reference/runtime 检查；保留这种观察/预测学习流程，不把已实现 baseline 称作未填学生代码，也不额外制造作业评分框架。

## 构建和运行

从 `Concurrency_Study/exercises` 执行：

```powershell
cmake -S M1_work_stealing_pool -B build/m1 -G "Visual Studio 18 2026" -A x64
cmake --build build/m1 --config Release
./build/m1/Release/M1_work_stealing_pool.exe
ctest --test-dir build/m1 -C Release --output-on-failure
```

统一构建里的 runtime target 由主线程自动注册。普通 Reference 有进程超时；不使用 sleep 推断顺序，不运行故意挂死的版本。

## Part 1：分工与逐项完整性

先预测前四分之一重、其余轻时，连续 static 分片的尾部阶段由谁工作；再追踪 dynamic 的原子 ID 领取与 stealing 的入队/出队。Reference 对三版本分别检查 N=0、1、257 的全部结果。

答案：static 可能把重任务集中给一个 worker；dynamic 用唯一 ID 领取把工作分配延后；stealing 则让空闲 worker 取走其他队列积压。三者都必须为每个 ID 填正确结果，不能只校验总和。P 大于 N 时允许空区间，P=0 必须拒绝。速度和 steals 比例没有必然顺序。

## Part 2：接纳、执行和错误通道

遮住 submit 和 run_one，按正文契约重写：外部提交轮转，本池 worker 提交到本地；owner 取 back，stealer 取 front。成功插入之后才增加 queued；取任务释放队列锁后再更新 queued/active。

Reference 用可移动独占资源的 lambda 检查 move-only callable，用抛出明确消息的任务检查 future 异常，再提交正常任务确认 worker 继续服务。答案：packaged_task 吸收用户异常进入结果状态，不在 worker 顶层泄漏；一个任务失败不意味着池关闭。分配失败发生在接纳前时应由提交者看到，不能留下虚假的任务计数。

runtime 对 400 个独立 ID 做 20 轮多生产者逐项计数检查，并验证 A 池 worker 向 B 池提交时，TLS 必须同时识别池身份。只保存 worker index 会在不同大小的池之间误用索引。

修订后的 runtime 还组合注入第二个 producer 创建失败与第一个活 producer 的真实 submit 分配失败，检查原始 system_error、记录的 bad_alloc，以及已接纳任务排空。共享 seen/errors 先声明，pool 随后，producer 容器最后，异常时按“提交者退出→池排空→数据销毁”收束。门闩在重抛前放行，不依赖 sleep；这不声称已经触发真实 OS 资源耗尽。

## Part 3：递归 fork-join

Reference 的 sum 把区间二分，左侧 submit，右侧本线程计算，最后 pool.wait(left)。cutoff 为 32，分别用一个和四个 worker 检查结果 8386560。

答案：单 worker 中裸 future.get 等未执行子任务会耗尽执行资源；wait 在等待时帮助执行 pending task。这个协议仅用于本池的良好嵌套 fork-join，不能等待 deferred/跨池 future，不能持有子任务需要的锁，也不能消除依赖环。外部 main 的 get 正常且推荐。代码不以 sleep 掩盖依赖错误。

## Part 4：关闭与生命周期

Reference 用 entered/release latch 证明：任务已经开始而队列已空时，shutdown future 仍不应就绪；放行任务后 drain 才完成，重复 join 安全，关闭后 submit 被拒绝。runtime 另让提交与 shutdown 竞争，逐 ID 检查只有成功接纳的任务执行，且恰好一次。

答案：排空必须同时看 closed、queued 和 active。shutdown future 只报告排空，不汇总任务异常，也不意味着 worker 已 join。worker 不能 join 自己的池；捕获对象必须活到任务不再使用它。线程创建部分失败时，构造 catch 先关闭、通知，再 join 已启动线程，最后重抛。这个 OS 失败路径本次未做故障注入，不能写成实测通过。

## Part 5：确认发生了窃取

Reference 最后使用两个 worker。root 在自己的队列提交 child，然后等 main 放行；main 必须收到 child 的完成，才释放 root，因此 child 必须被另一个 worker 从 root 队列取走，检查 steals 至少一次。提交失败会通过 promise 通知 main 并解除等待。

答案：这个握手专门保留第二个执行槽位，是有限协议检查。不能把“在任务中阻塞等待也能跑完这个测试”推广到任意数量的父子任务。通常不应断言本地命中远多于窃取，实际比例取决于工作分布。

## 基准与记录

[scheduling_bench.cpp](../benchmarks/scheduling_bench.cpp) 接受 `--variant static|dynamic|stealing --size N --threads P`，一次只输出选中版本的统一 CSV。计时包含分配、线程构造、调度、计算和 join；逐项检查在计时后。用公共 runner 采样，不要求特定加速。

作者已在 MSVC 19.51 Release 下运行 Reference 与 runtime；后续改动的验证记录见[专题验证记录](../../topics/scheduling/verification.md)。独立 review 由主线程另行组织，不把作者自查写成独立验收。
