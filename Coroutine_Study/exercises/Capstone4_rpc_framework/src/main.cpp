// =============================================================================
// src/main.cpp —— RPC demo driver
//
// 对应文档：13-第三阶段结课-RPC框架.md  §"必做任务 7"
//
// 测试场景：
//   - 启动 server 在后台线程；
//   - Client 同时发起 5 个并发请求：
//       * 3 个调用 add，正常完成；
//       * 1 个调用 delay_add，超时 100ms < 500ms 实际延迟 -> Timeout；
//       * 1 个调用 error_method -> ServerError completion；
//   - 用 async_scope 收束所有 in-flight 请求；
//   - 验证：共拿到 5 个结果（3 ok / 1 timeout / 1 error）。
//
// 设计验收：
//   - 没有 detach；
//   - 没有全局可变状态；
//   - scope 的析构 / on_empty() 能正确等待所有 in-flight；
//   - cancellation token 从 client 超时到 server handler 的传播路径清晰。
// =============================================================================

#include "rpc/protocol.hpp"
#include "rpc/scope.hpp"
#include "rpc/stop_token.hpp"
#include "rpc/task.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

// 真实实现需要：
// #include <stdexec/execution.hpp>
// #include <asio.hpp>
// #include "rpc/client.hpp"  // 上层暴露 RpcClient 接口
// #include "rpc/server.hpp"  // 上层暴露 RpcServer 接口

using namespace std::chrono_literals;

// =============================================================================
// 帮助函数：把一个 Response 打印为人类可读
// =============================================================================
static void print_outcome(std::size_t i, std::string label, std::string detail) {
    std::cout << "  [" << i << "] " << label << ": " << detail << "\n";
}

int main() {
    std::cout << "===== Capstone 4: mini RPC Framework demo =====\n";

    // -------------------------------------------------------------------------
    // 1. 启动 server（在后台线程或 io_context::run() 中）
    //    TODO[必做]: 创建 RpcServer，注册 handlers，调用 serve(port)。
    // -------------------------------------------------------------------------
    // RpcServer server;
    // std::jthread server_thread([&] { sync_wait(server.serve(9876)); });

    // -------------------------------------------------------------------------
    // 2. 创建 client
    //    TODO[必做]: 连接 127.0.0.1:9876
    // -------------------------------------------------------------------------
    // RpcClient client;
    // sync_wait(client.connect("127.0.0.1", 9876));

    // -------------------------------------------------------------------------
    // 3. 5 个并发请求
    //    用 when_all 收束；其中 1 个超时、1 个 error_method、3 个 add。
    // -------------------------------------------------------------------------
    // auto outcomes = sync_wait(when_all(
    //     client.call({.method="add",          .args={1, 2}},   3s),
    //     client.call({.method="add",          .args={10, 20}}, 3s),
    //     client.call({.method="add",          .args={100,200}},3s),
    //     client.call({.method="delay_add",    .args={500}},    100ms),  // timeout
    //     client.call({.method="error_method", .args={}},       3s)      // error
    // ));

    // -------------------------------------------------------------------------
    // 4. 打印结果
    // -------------------------------------------------------------------------
    print_outcome(0, "add{1,2}",         "ok 3 (TODO)");
    print_outcome(1, "add{10,20}",       "ok 30 (TODO)");
    print_outcome(2, "add{100,200}",     "ok 300 (TODO)");
    print_outcome(3, "delay_add{500}",   "Timeout (TODO)");
    print_outcome(4, "error_method{}",   "ServerError (TODO)");

    // -------------------------------------------------------------------------
    // 5. 关停：等待 client.scope().on_empty()，再 shutdown server
    // -------------------------------------------------------------------------
    // sync_wait(client.shutdown());
    // server_thread.request_stop();

    std::cout << "===== Done =====\n";
    return 0;
}
