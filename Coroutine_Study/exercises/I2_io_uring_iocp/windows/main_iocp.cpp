// I-2 (Windows) IOCP awaiter
// 文档参考：11-模块I-真实异步IO与并发框架.md 「练习 I-2」
// 官方参考：
//   - Microsoft IOCP: https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports
//   - WSARecv/WSASend overlapped IO
//
// 目标：为 Windows IOCP 写最小 awaiter——把 OVERLAPPED 嵌入 awaiter 作为第一个成员，
//      使 OVERLAPPED* == awaiter*。await_suspend 里调 WSARecv 投递请求，
//      工作线程从 GetQueuedCompletionStatus 取出 OVERLAPPED*，reinterpret_cast
//      回 awaiter，再 resume 协程。

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <coroutine>
#include <cstdio>
#include <cstring>
#include <exception>
#include <utility>

#pragma comment(lib, "ws2_32.lib")

// ============ iocp_loop ============
struct iocp_loop {
    HANDLE iocp = INVALID_HANDLE_VALUE;
    bool   running = true;
    int    pending = 0;

    iocp_loop() {
        iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
        if (!iocp) {
            std::fprintf(stderr, "CreateIoCompletionPort failed: %lu\n",
                         GetLastError());
            std::abort();
        }
    }
    ~iocp_loop() { if (iocp) CloseHandle(iocp); }
    iocp_loop(const iocp_loop&) = delete;
    iocp_loop& operator=(const iocp_loop&) = delete;

    void bind(SOCKET s) {
        CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), iocp, 0, 0);
    }

    void run() {
        while (running && pending > 0) {
            DWORD       transferred = 0;
            ULONG_PTR   key = 0;
            OVERLAPPED* ov = nullptr;
            BOOL ok = GetQueuedCompletionStatus(iocp, &transferred, &key, &ov, INFINITE);
            if (!ov) {
                // 退出哨兵或超时
                continue;
            }
            // 关键：OVERLAPPED 是 awaiter 第一个成员，OVERLAPPED* 就是 awaiter*
            auto* base = reinterpret_cast<awaiter_base*>(ov);
            base->transferred_ = transferred;
            base->success_ = ok ? true : false;
            base->error_ = ok ? 0 : GetLastError();
            --pending;
            base->coro_.resume();
        }
    }

    // 让事件循环退出
    void stop() {
        running = false;
        PostQueuedCompletionStatus(iocp, 0, 0, nullptr);
    }

    // ============ awaiter 基类：OVERLAPPED 必须是第一个成员 ============
    struct awaiter_base {
        OVERLAPPED ov_{};                       // 第一个成员 —— 必不可改
        std::coroutine_handle<> coro_;
        DWORD transferred_ = 0;
        DWORD error_       = 0;
        bool  success_     = false;
    };
};

static_assert(offsetof(iocp_loop::awaiter_base, ov_) == 0,
              "OVERLAPPED must be the first member of awaiter_base for the "
              "OVERLAPPED* == awaiter* trick to hold");

// ============ iocp_recv_awaiter ============
struct iocp_recv_awaiter : iocp_loop::awaiter_base {
    iocp_loop* loop_;
    SOCKET     socket_;
    char*      buf_;
    DWORD      len_;

    iocp_recv_awaiter(iocp_loop* loop, SOCKET s, char* buf, DWORD len) noexcept
        : loop_(loop), socket_(s), buf_(buf), len_(len) {
        std::memset(&ov_, 0, sizeof(ov_));
    }

    bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<> h) noexcept {
        coro_ = h;
        WSABUF wbuf{len_, buf_};
        DWORD flags = 0;
        int rc = WSARecv(socket_, &wbuf, 1, nullptr, &flags, &ov_, nullptr);
        if (rc == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err != WSA_IO_PENDING) {
                error_ = err;
                success_ = false;
                // 同步失败：交给事件循环 resume 自身（投递一个 completion）
                PostQueuedCompletionStatus(loop_->iocp, 0, 0, &ov_);
            }
        }
        ++loop_->pending;
    }

    int await_resume() noexcept {
        if (!success_) return -static_cast<int>(error_);
        return static_cast<int>(transferred_);
    }
};

struct iocp_task {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        // 关键：必须把协程帧 handle 交给 iocp_task 持有，否则 final_suspend 为
        // suspend_always 时谁都不 destroy()，协程帧泄漏。
        iocp_task get_return_object() noexcept {
            return iocp_task{handle_type::from_promise(*this)};
        }
        std::suspend_never  initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend()   noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { std::terminate(); }
    };

    handle_type h_{};
    explicit iocp_task(handle_type h) noexcept : h_(h) {}
    iocp_task(iocp_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    iocp_task& operator=(iocp_task&& o) noexcept {
        if (this != &o) { if (h_) h_.destroy(); h_ = std::exchange(o.h_, {}); }
        return *this;
    }
    iocp_task(const iocp_task&) = delete;
    iocp_task& operator=(const iocp_task&) = delete;
    // 此时协程已跑到 final_suspend（suspend_always），destroy() 行为良定义。
    ~iocp_task() { if (h_) h_.destroy(); }
};

iocp_task echo_once(iocp_loop& loop, SOCKET sock) {
    char buf[1024];
    int n = co_await iocp_recv_awaiter{&loop, sock, buf, sizeof(buf)};
    if (n > 0) {
        std::printf("[iocp] received %d bytes: ", n);
        ::fwrite(buf, 1, static_cast<size_t>(n), stdout);
    } else {
        std::printf("[iocp] recv failed (err=%d)\n", -n);
    }
    co_return;
}

int main()
{
    std::printf("===== I-2 (Windows) IOCP awaiter =====\n");

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    // 创建 listening socket，等一个客户端连接，然后 co_await 一次 recv
    SOCKET listener = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0,
                                 WSA_FLAG_OVERLAPPED);
    if (listener == INVALID_SOCKET) {
        std::fprintf(stderr, "WSASocket(listener) failed\n");
        WSACleanup();
        return 1;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(12345);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    listen(listener, 1);

    std::printf("[main] waiting for client on 127.0.0.1:12345\n");
    SOCKET client = accept(listener, nullptr, nullptr);
    closesocket(listener);

    iocp_loop loop;
    loop.bind(client);

    auto task = echo_once(loop, client);  // 持有协程帧；立即挂起在 co_await
    loop.run();                            // 事件循环驱动 completion → resume
    // task 在 main 结束时析构 → 协程帧在 final_suspend 处被 destroy()，无泄漏

    closesocket(client);
    WSACleanup();

    // TODO [必做]：用 nc 127.0.0.1 12345 发一行字符串验证 recv 返回。
    // TODO [必做]：在笔记中说明 OVERLAPPED* == awaiter* 的成立前提。
    // TODO [进阶]：实现 iocp_send_awaiter，写完整 echo 循环。
    // TODO [进阶]：用 RIO 替换 WSARecv 模型，对比延迟。

    std::printf("\n===== Done =====\n");
    return 0;
}
