# P1 有界 HTTP/WebSocket 任务服务

正文：[HTTP/WS](../../chapters/07-http-websocket.md)、[任务服务](../../chapters/11-task-service.md)。已提供 [server.hpp](server.hpp)、[client.hpp](client.hpp)、[main.cpp](main.cpp)完整 Reference。配置固定 Boost 即可，目标 C11_P1_service 是有限自检服务，启动回环临时端口并自动收束。

## Part A：逐层追踪一次任务

追踪 POST /tasks → parse_spec → engine.submit → registry.admit → worker slot → registry.finish → GET/WS。提交 JSON 包含 left/right，steps 默认0、budget_ms 默认5000；header 必须有一个 Idempotency-Key 和 application/json。先手工画 snapshot 与 live/retained 的变化，再运行。

```powershell
cmake --build build/c11-asio --config Release --target C11_P1_service
ctest --test-dir build/c11-asio -C Release -R '^C11_P1_service$' --output-on-failure
```

## Part B：重复、容量与取消

main 将 live 上限收紧为2并用测试 gate 固定运行状态。重复同 spec/key 应返回同 ID，不同 spec 冲突；第三个新任务拒绝。取消 running 后 gate 未释放时仍满载；释放后终态 cancelled，另一个任务成功42。WS 先发当前快照，再按递增 revision 到终态，然后 close。

解析：重复提交覆盖“响应丢失后重试需要的领域行为”，本驱动没有在网络层真的丢弃第一次响应，不能据检查文字误称损伤实验。取消请求只记录意图，worker 完成才释放槽位。终态消息进入网络写不表示远端已消费，单写者与缓冲保活继续负责直到完成。

## Part C：独立修改作业

复制 server.hpp/main.cpp 到自己的练习分支，增加一个只读计数响应字段，或为合法新提交接入 L10 token bucket。保留原文件作 Reference。要求明确限流放在重复键重用之前还是之后：若重复也收费，必须写成新的契约；不能意外让满载重试丢失原任务。

解析：保留 key 查询与冲突验证后，对新 create 计费最容易保持现有重试语义。获取令牌失败不能增加 live、accepted 或创建新 ID；配置限流后需要修改相应观察输入，而非降低原容量/关闭检查。

当前项目不提供持久化、多租户授权、HTTPS 接线或公网常驻启动模式。它完整展示已声明的内存任务服务契约，后续生产能力按明确需求加入。
## IDE 工程入口

VS solution 中本题主入口是 `C11_P1_service`。本单元是观察/专项入口，没有学生占位；源码、README、协议文件或脚本显示在同一项目中，依赖目标保留为独立项目。程序通过只证明本驱动运行，不代替 README 要求的预测、解释或专项依赖准备。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
