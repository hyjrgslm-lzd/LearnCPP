# B01 连接复用的成本分解

正文：[测量方法](../../chapters/12-bridges-source-performance.md)。复用 L10 的真实池和 L04 Reactor；本单元没有另写一份“只用于跑分”的协议。

## 运行

默认目标不参与 ALL，显式构建后运行有限样本。每方案预热1次、独立进程样本5次，每次100请求、128字节body，所有响应核对一致，超时或清理失败停止比较。

```powershell
cmake --build build/c11-windows --config Release --target C11_B01_pool_cost
python C11_Networking_Service_Design/exercises/B01_pool_cost/sample.py build/c11-windows/B01_pool_cost/Release/C11_B01_pool_cost.exe --output build/c11-pool-samples.json
```

单次可用 `C11_B01_pool_cost.exe fresh 100` 或 `pool 100`；请求上限1000，内部总预算20秒。正式记录用 sample.py 的外部超时，输出文件必须是新文件。

## Part A：预测与计时口径

先预测 connections=N 与1，再比较 acquire_us、transfer_us、work_us、drain_us。初始化和线程启动不计 work；每轮 socket析构/租约归还计入 work，但不全计进 acquire/transfer；最终池关闭和owner退出独立计 drain。

## Part B：解释实际结果

只根据成功回合计算中位数并保留每个样本。若 acquire 减少而 work 不明显改变，说明该负载的其他阶段占主导；不能仍发布按连接数计算的加速比。若结果不稳定，增加样本前先核对请求量、错误、调度和计时边界。

解析：这是一个顺序闭环客户端，测得的是此条件下连接策略成本，不是固定到达率的尾延迟或公网吞吐。假设“池更快”可以被推翻，失败/超时样本不能当有效慢样本混入，也不能删除后继续声称全部成功。改变并发、TLS 或消息尺寸时另建场景，不与当前结果直接混排。
