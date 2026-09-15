# L10 重试、令牌桶与连接池

正文：[服务策略](../../chapters/10-rpc-service-policy.md)。默认不需要第三方依赖。

## Part A：实现有限重试

只编辑 [student/solution.hpp](student/solution.hpp)中的 retry(context,rng)。已提供时钟、输入类型、token_bucket 和检查器。先验证 idempotent/transient/max_attempts/attempt/deadline/Retry-After，再计算 full jitter。不要修改随机种子或检查器来让恒定返回通过。

```powershell
cmake -S C11_Networking_Service_Design/exercises/L10_service_policy -B build/c11-l10 -DC11_TEST_STUDENTS=ON
cmake --build build/c11-l10 --config Release --target C11_L10_student
ctest --test-dir build/c11-l10 -C Release -R '^C11_L10_student$' --output-on-failure
```

[Reference](reference/solution.hpp)使用 [service_policy.hpp](../include/c11/service_policy.hpp)，good 独立实现，bad 故意违反预算/策略。解析：总尝试包含第一次；退避为 25ms 指数增长、封顶 500ms 的全抖动，再尊重 Retry-After。delay 大于等于剩余预算时拒绝重试。零总次数先拒绝，避免减一溢出。

## Part B：令牌桶

读取 checks.cpp 的显式时钟输入，预测突发耗尽、周期补充、时间倒退和过大 cost。解析：信用上限控制突发，补充周期控制平均率，倒退不增加信用；并发上限另由任务接纳控制。此题不把虚拟时钟注入称作真实等待测量。

## Part C：真实连接池

[pool.cpp](pool.cpp)复用同一 loopback endpoint，观察两次请求只建一个连接、空闲到期替换、有限等待队列、等待超时和 close。运行 C11_L10_pool。

解析：只有完整响应校验成功才 reusable；租约默认丢弃。close 唤醒等待者但保留借出的 socket，最后租约归还后才能完全关闭。等待状态由通知确认，TTL 用显式维护时间注入，不靠 sleep 猜测。性能比较另在 B01，不从功能通过宣称池更快。
## IDE 工程入口

VS solution 中本题主入口是 `C11_L10_student`。学生只编辑 `student/solution.hpp`；`checks.cpp` 是共同检查器，Reference/good/bad 和额外 bad 控制保持独立项目，用来区分答案、正确对照和错误拒绝。单题可用 `cmake -S <本目录> -B <build>` 独立生成，`C11_TEST_STUDENTS` 只控制是否把未完成 Student 注册进 CTest。
