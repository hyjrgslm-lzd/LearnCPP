# R1：task 与 counting scope 的实际接口观察

这是完整可运行的观察单元，起点是 `main.cpp`，没有伪造的 Student/Reference 空目录。先读[task 与 scope 正文](../../chapters/10-task-and-scope.md)，再沿固定 stdexec 源码核对对象和状态；程序成功只验观察条件，不代替读者的解释作业。

## Part 1：关联、关闭和 join

运行 simple scope 场景前，预测 first/second 两个变量及 late 结果。首次 associate 在入口开放时取得关联并执行；close 之后的新关联被拒绝，late 没有 value，second 不会变成 true。join 的完成意味着需要等待的关联已经释放，不能用“sender 变量出了作用域”代替。

手工持有 `try_associate()` 返回的关联对象，再从另一条 jthread 等待 join。释放关联对象后等待线程才可完成。画出 scope、token、关联对象、join operation、等待线程的寿命；jthread 在互斥量和条件变量之前析构并真正 join，保证通知端不在局部同步对象销毁后继续运行。

## Part 2：停止与拒收的差异

counting_scope 提供 stop source；request_stop 后，已关联 sender 的 `read_env(get_stop_token)` 可以观察请求。这个实验没有假设请求必然让整个 sender stopped：它读取 token 状态并以 value 返回。与 Part1 的 close 拒收比较，解释停止请求、是否接受关联、最终完成三个独立事件。

## Part 3：spawn 与 spawn_future

spawn 运行附着于 scope token 的工作；示例以原子计数观察执行，再 join。spawn_future 除关联/执行之外还提供携带结果的 future sender；消费该 sender 取得42，之后仍等待 scope 完成。future 的结果访问与整个作用域的资源收束是两项责任。

当前 `stdexec::` 是固定参考实现的标准接口方向；`exec::task`、`exec::async_scope` 为扩展层。S1/正文进一步比较具体用法，F01 单独检测工具链真正的 `std::execution`，不能把本单元通过当作原生标准库支持。

## 验收与解析

按[构建指南](../BUILD_GUIDE.md)以 `R1_task_scope` 为独立 source 目录；目标/测试同名。所有观察使用 `c10::require`，Release仍有效。写下上述三个Part的预测、对象图、固定源码位置及观察解释；在不改变既有条件的情况下增加一个显式关闭后关联的案例，可验证自己的理解。

本单元不证明上游库所有交错正确，不承诺强制取消任意用户函数，也不测吞吐。把同样的所有权关系带入 P1 的文件、buffer和CPU分支，再讨论错误或取消发生时谁负责等待。
