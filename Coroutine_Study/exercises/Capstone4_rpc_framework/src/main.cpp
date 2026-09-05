// =============================================================================
// src/main.cpp —— RPC demo driver
//
// 对应文档：13-第三阶段结课-RPC框架.md  §"必做任务 7"
//
// 本文件是学生 starter，不冒充已完成实现。
// 完整可运行答案在 reference/tests/rpc_reference_test.cpp。
//
// 设计验收：
//   - 没有 detach；
//   - 没有全局可变状态；
//   - completion handler 成对维护计数，shutdown 调 wait_empty() 完成 drain；
//   - cancellation token 从 client 超时到 server handler 的传播路径清晰。
// =============================================================================

#include "rpc/protocol.hpp"
#include "rpc/scope.hpp"
#include "rpc/task.hpp"

#include <iostream>

// #include "rpc/client.hpp"  // 上层暴露 RpcClient 接口
// #include "rpc/server.hpp"  // 上层暴露 RpcServer 接口

int main() {
    std::cout << "===== Capstone 4: mini RPC Framework demo =====\n";
    std::cout << "starter skeleton: pure Asio awaitable RPC, not a completed implementation.\n";
    std::cout << "exercise shape: 3 ok calls + timeout + server_error + unknown_method.\n";
    std::cout << "ownership rule: every co_spawn has a completion handler or a stored future.\n";
    std::cout << "drain rule: shutdown waits until client/server in_flight() is zero.\n";
    std::cout << "reference check: build target Capstone4_rpc_framework_reference.\n";
    return 0;
}
