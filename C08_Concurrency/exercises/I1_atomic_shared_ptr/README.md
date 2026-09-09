# I1：不可变快照与多写者更新

完整正文见[不可变快照与多写者更新](../../topics/atomics/07-publication-and-lifetime.md)。[main.cpp](main.cpp) 是安全、有限结束的 Starter；[solution.cpp](solution.cpp) 是含运行检查的 Reference。默认 C++23。

## Part 1：替换入口后保留旧对象

Starter 已用 atomic<shared_ptr<const config>> 发布不可变对象。Reference 持有第 0 版，发布第 10 版，用 weak_ptr 检查旧版未过期；释放最后强引用后检查过期。

答案：原子 load 取得一份拥有所有权的副本，相关计数增加是原子操作的一部分；acquire 取得发布内容。控制块安全不表示 T 任意字段可并发修改，Reference 通过 make_shared<const config> 从构造起保持不可变，不留 mutable 别名。

## Part 2：替换与累计更新必须分开

让两个调用者都从第 10 版计算第 11 版，分别 store。Reference 检查最终为 11，说明两次替换不等于两次增量。这是有定义的业务丢更新。

随后实现 `increment(cell)`：acquire load old，构造新 const 对象，acq_rel CAS；失败用 acquire 回填的 old 重算，再构造 next。

答案：成功 CAS 是增量生效点；失败回填既拥有寿命保护又取得内容同步。不能把 old 更新后仍提交旧 next，也不能给可能重复执行的构造步骤添加不可重放外部副作用。

## Part 3：并发自洽性和最终更新量

两写者各执行 1000 次增量，两读者各做 3000 次读取并 cs::check(payload==version*2)。全部 get 后，主线程检查 version=2000、payload=4000。worker 中的分配失败与检查失败通过 future 回传；其他任务不等待失败任务发信号，因此异常不会让一个协议确认永久缺席。

答案：读者可跳过中间版本，不要求观察到最终版。读者必须持有 shared_ptr 直到访问完毕，不能取裸指针后立即销毁副本。打印 is_lock_free，无论 true/false 都不据此排名；整个更新还包括分配、引用计数和竞争重试。

快照不是逐版投递队列，也不自动设定历史版本内存上限：长期持有的读者会延长旧版本寿命。析构与释放不全部位于原子更新步骤内，需纳入真实应用的延迟和退出设计。

## 构建与验收

从 `C08_Concurrency/exercises` 执行：

```powershell
cmake -S I1_atomic_shared_ptr -B build/I1_atomic_shared_ptr -G "Visual Studio 18 2026" -A x64
cmake --build build/I1_atomic_shared_ptr --config Release
./build/I1_atomic_shared_ptr/Release/I1_atomic_shared_ptr.exe
ctest --test-dir build/I1_atomic_shared_ptr -C Release --output-on-failure
```

VS2026 生成器需要 CMake 4.2 或更新版。CTest 运行 `I1_atomic_shared_ptr_reference` 并设进程超时；cs::check 在 Release 仍有效。通过表示本次检查成功，不替代正文中的协议证明。规范链接与版本说明见对应正文。
