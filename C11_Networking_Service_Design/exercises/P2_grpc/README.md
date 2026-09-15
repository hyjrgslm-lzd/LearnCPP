# P2 同一领域实现的 gRPC 适配

正文：[RPC](../../chapters/10-rpc-service-policy.md)、[任务服务](../../chapters/11-task-service.md)。与 P1 共用 C++ registry/runtime，但为独立进程实例，不共享任务状态。固定 gRPC/Protobuf 来源与 Windows 命令见 [BUILD_GUIDE](../BUILD_GUIDE.md)。

## Part A：schema 与 unary

[tasks.proto](tasks.proto)定义 Submit/Get/Cancel/Watch。解释 optional left/right/budget_ms，观察 protoc 生成的访问器及 grpc plugin 生成的 Stub/Service。不要编辑 build 中的生成文件。新增字段用新编号，旧编号不能重新解释。

```powershell
cmake --build build/c11-grpc --config Release --target C11_P2_grpc
ctest --test-dir build/c11-grpc -C Release -R '^C11_P2_grpc$' --output-on-failure
```

## Part B：请求与任务取消

[service.hpp](service.hpp)提供同步适配基线，[main.cpp](main.cpp)在实际 HTTP2/TLS 上检查 Submit、重复、冲突、Watch、Cancel、deadline、错误 hostname 与完整关闭。跟踪 invoke 的值捕获、packaged_task、future、active 标记；列出 RPC 已返回时 owner 是否仍可能执行。

解析：若 owner action 尚未开始且 active=false，它拒绝执行；若已接纳，则请求取消不回滚任务。ClientContext deadline 与任务 budget 独立。Watch 的 alive 守卫结束观察，定时 guard 在 ServerContext 失效前 join，避免捕获失效指针。同步模型线程数有上限，不是无限 stream 方案。

## Part C：错误映射

把领域 invalid/conflict/not_found/overloaded 映射到 INVALID_ARGUMENT/ALREADY_EXISTS/NOT_FOUND/RESOURCE_EXHAUSTED，与 P1 HTTP 码对照。解析：业务错误保留含义，传输 EOF 不应伪装成任务 failed；服务 stopping 用 UNAVAILABLE，deadline 与 CANCELLED 也不能当幂等重试许可。

Reference 为本目录三份源与共享 runtime。BoringSSL 随固定 gRPC 构建，与 P1/L07 OpenSSL 和 MsQuic TLS 保持不同可执行程序，避免把同名加密符号混链。应用4KiB消息上限、128调用、32订阅与 transport quota 各有边界，不能声称 quota 等于进程 RSS 硬上限。
## IDE 工程入口

VS solution 中本题主入口是 `C11_P2_grpc`。本单元是观察/专项入口，没有学生占位；源码、README、协议文件或脚本显示在同一项目中，依赖目标保留为独立项目。程序通过只证明本驱动运行，不代替 README 要求的预测、解释或专项依赖准备。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
